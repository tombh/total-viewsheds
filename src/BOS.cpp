#include <plog/Log.h>

#include "BOS.h"
#include "DEM.h"
#include "definitions.h"

namespace TVS {

/**
 * The Band of Sight (BoS) is essentially the line of sight. In other
 * words: which DEM points fall along a line 'as the crow flies'.
 *
 * The conception of the BoS here is different to traditional 'V'-shaped
 * methods. Here we use a parallel band. This is fundamental to the
 * optimization improvements found in this whole approach.
 *
 * Note that not all band data is saved for later computation. Only bands that
 * are able to see both forward and backward to the extent of the maximum line of
 * sight flag are saved for later. However, *all* possible bands need to be
 * computed in order to find those bands.
 *
 * A useful image to remember when working with this code is that the BoS doesn't
 * rotate like a light house, but rather sweeps, assimilating one new point at a
 * time. This is in fact fundamental to the efficiency improvements of this algorithm.
 * A naive solution would rotate about every point and then move onto the next point.
 * Whereas here, we generate an entirely new BoS simply by adding just one new point
 * per iteration. Then, once the entire DEM has been swept, we rotate, repeating the
 * same process for a new angle.
 *
 * Consider the following DEM, where '/' represents the sides of a single BoS:
 *
 *  Start        Front
 *    <  .  .  .  +  /  .  .  .
 *    .  .  .  /  +  .  .  .  .
 *    .  .  -  +  /  .  .  .  .
 *    .  .  /  +  .  .  .  .  .
 *    .  . (+) /  .  .  .  .  .
 *    .  /  +  .  .  .  .  .  .
 *    .  +  /  .  .  .  .  .  .
 *    / [+] @  .  .  .  .  .  .
 *    +  /  .  .  .  .  .  .  >
 *   Back                    End
 *
 * Even though '+' points are part of the BoS, they are not necessarily points
 * on a perfectly straight line, yet they do have increasing orthogonal distances
 * from the "Front" and "Back". Point '[+]' was just added and '@' will be the
 * next point added -- but in order to do so, a point will need to be removed,
 * just as '-' was during the previous BoS and '(+)' in the next. "Start" and
 * "End" represent the sector ordering, they are where the BoS begins and finishes.
 *
 * Conveniently the final computable band arrays are highly compressable. After all
 * they are straight(ish) lines over a grid, so the numeric differences between each DEM
 * ID tend to have many repetitions. This is exploited, giving benefits of reduced
 * precomputation data footprint and faster bandwidth transfer to the GPU.
**/
BOS::BOS(DEM &dem)
    : dem(dem),
      band_size(ensureBandSizeIsOdd(FLAGS_dem_width)),
      // The half band helps decide which points to add next during sector sweep.
      half_band_size((band_size - 1) / 2),
      // Limit and standardise final viewshed computations. The limit
      // is used to restrict the area search for the viewshed. Whereas
      // having a consistent size of band is useful for vectorised
      // parallel computation.
      computable_band_size((FLAGS_max_line_of_sight / FLAGS_dem_scale) + 1),
      sector_angle(-1),
      // The actual Band of Sight data structure.
      bos(LinkedList(band_size)) {}

BOS::~BOS() {
  delete[] this->band_markers;
  if (this->dem.is_computing) {
    delete[] this->band_data;
  }
}

// Currently, adjusting to new BoS points requires the Bos itself to have
// an odd size. Roughly speaking, think of it like this:
//
//   BoS = half_band_size + PoV + half_band_size
//
// It's easier to just override band_size than rewrite the bulk of the code
// to support even-sized band sizes.
int BOS::ensureBandSizeIsOdd(int requested_band_size) {
  if (requested_band_size % 2 == 0) {
    requested_band_size++;
    LOGI << "Raising `band_size` from " << requested_band_size - 1 << " to " << requested_band_size;
  }
  return requested_band_size;
}

// TODO: rename to something about band data
void BOS::openPreComputedDataFile() {
  char path[100];
  const char *mode;
  this->preComputedDataPath(path, this->sector_angle);
  if (this->dem.is_precomputing) {
    mode = "wb";
  } else {
    mode = "rb";
  }
  this->precomputed_data_file = fopen(path, mode);
  if (this->precomputed_data_file == NULL) {
    LOG_ERROR << "Couldn't open " << path << " for sector_angle: " << this->sector_angle;
    throw "Couldn't open file.";
  }
}

void BOS::preComputedDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", SECTOR_DIR.c_str(), sector_angle);
}

// Set the band of sight up for the very first time, namely after a sector angle
// change.
void BOS::setup(int angle) {
  this->adjustToAngle(angle);
  this->sector_ordered_id = 0;
  this->pov = 0;
  this->bos.Clear();
  this->bos.FirstNode(this->dem.sector_ordered[0]);
}

void BOS::adjustToAngle(int angle) {
  if (this->dem.is_computing && this->sector_angle != -1) {
    delete[] this->band_data;
  }
  this->sector_angle = angle;
  this->openPreComputedDataFile();
  if (this->dem.is_precomputing) {
    // Skip to after the band markers
    fseek(this->precomputed_data_file, (this->dem.computable_points_count * 2 * 4) + 4, SEEK_SET);
  }
  this->band_data_size = 0;
}

// TODO: support even band sizes.
// Anchoring to the PoV requires managing the edges of the BoS by carefully
// using the half_band_size. This has the disadvantage of only supporting
// odd band sizes.
void BOS::adjustToNextPoint(int sector_ordered_id) {
  this->sector_ordered_id = sector_ordered_id;
  this->pov = this->sector_ordered_id % this->band_size;

  // Is the band of sight starting to be populated?
  bool starting = this->sector_ordered_id < this->half_band_size;

  // Is the band of sight coming to an ending, therefore emptying?
  int end_section = this->dem.size - this->half_band_size - 1;
  bool ending = this->sector_ordered_id >= end_section;

  int leading;
  int doubled = 2 * this->sector_ordered_id;

  if (starting) {
    this->remove = false;

    this->new_point = this->dem.sector_ordered[doubled + 1];
    this->insertPoint();

    this->new_point = this->dem.sector_ordered[doubled + 2];
    this->insertPoint();
  }

  if (!starting && !ending) {
    this->remove = true;
    leading = this->sector_ordered_id + this->half_band_size + 1;
    this->new_point = this->dem.sector_ordered[leading];
    this->insertPoint();
  }

  if (ending) {
    this->bos.Remove_one();
    this->bos.Remove_one();
  }
}

// Where does the new point fit in the current BoS?
int BOS::calculateNewPosition() {
  int position;
  int sweep;
  int sanity = 0;
  int current_sight_index = this->dem.sight_ordered[this->new_point];
  int first_sight_index = this->dem.sight_ordered[this->bos.LL[this->bos.First].dem_id];
  int last_sight_index = this->dem.sight_ordered[this->bos.LL[this->bos.Last].dem_id];

  if (current_sight_index < first_sight_index) {
    position = -2;
  } else if (current_sight_index > last_sight_index) {
    position = -1;
  } else {
    sweep = this->bos.LL[this->bos.First].next;
    while (current_sight_index >= this->dem.sight_ordered[this->bos.LL[sweep].dem_id]) {
      sweep = this->bos.LL[sweep].next;
      if (sanity > this->dem.size) {
        LOGE << "new_point sight_ordered index: " << new_point << " is bigger than anything in band of sight.";
        throw;
      }
    }
    position = this->bos.LL[sweep].prev;
  }
  return position;
}

// Insert a DEM point into the BoS linked list.
// TODO: Refactor into the LinkedList class.
void BOS::insertPoint() {
  int position = this->calculateNewPosition();
  if (position > -1) {
    this->bos.Add(this->new_point, position, this->remove);
  } else {
    if (position == -1) {
      this->bos.AddLast(this->new_point, this->remove);
    }
    if (position == -2) {
      this->bos.AddFirst(this->new_point, this->remove);
    }
  }
}

// The band data file consists of:
// * 1 x TVS-sized integer array for front band markers.
// * 1 x TVS-sized integer array for back band markers.
// * A single integer representing the size of the compressed data that follows.
// * Arbitary length array of short integers. Individual bands can be found from
//   the markers at the head of the file. Arrays are in pairs of 'count:delta',
//   where 'count' is the number of occurences of the 'delta', which is the
//   difference between the previous band DEM ID and the current.
void BOS::initBandStorage() {
  this->band_markers = new int[this->dem.computable_points_count * 2];
}

// Sweep through the current BoS linked list to create a normal array, from
// which we can compress the deltas using RLE, ie. '22233444' to '3:2,2:3,3:4'.
// These compressed arrays are saved to file and later sent to the GPU for parallel
// compuation.
// TODO: The while() loop here and in calculateNewPosition() have similarities,
// is there a performance gain to be found by somehow combining them?
void BOS::buildComputableBands() {
  int pov_id = this->bos.LL[this->pov].dem_id;
  int tvs_id = this->dem.povIdToTVSId(pov_id);
  CompressedBand front_band = CompressedBand();
  CompressedBand back_band = CompressedBand();
  front_band.delta_count = 0;
  back_band.delta_count = 0;
  front_band.pointer = 0;
  back_band.pointer = 0;
  front_band.data = new short[this->computable_band_size * 2];
  back_band.data = new short[this->computable_band_size * 2];

  int count = 1;
  int sweep_front = this->bos.LL[this->pov].next;
  int sweep_back = this->bos.LL[this->pov].prev;
  front_band.current_delta = this->bos.LL[sweep_front].dem_id - pov_id;
  back_band.current_delta = this->bos.LL[sweep_back].dem_id - pov_id;
  front_band.previous_id = pov_id;
  back_band.previous_id = pov_id;
  while (count < this->computable_band_size) {
    this->compressBand(front_band, this->bos.LL[sweep_front].dem_id);
    this->compressBand(back_band, this->bos.LL[sweep_back].dem_id);
    sweep_front = this->bos.LL[sweep_front].next;
    sweep_back = this->bos.LL[sweep_back].prev;
    count++;
  }

  // Close the compressed band by forcing a delta change, thus ensuring that the
  // current delta gets written as the final RLE pattern. `pov_id` forces a delta
  // change because it is impossible that the difference between the last ID and the
  // penultimate ID is the same as the difference between the last ID and the first
  // (ie PoV) ID.
  this->compressBand(front_band, pov_id);
  this->compressBand(back_band, pov_id);

  // Front band starts here
  this->band_markers[tvs_id] = this->band_data_size;
  // The current back band starts here
  this->band_data_size += front_band.pointer;
  this->band_markers[tvs_id + this->dem.computable_points_count] = this->band_data_size;
  // The next front band will start here
  this->band_data_size += back_band.pointer;

  fwrite(front_band.data, 2, front_band.pointer, this->precomputed_data_file);
  fwrite(back_band.data, 2, back_band.pointer, this->precomputed_data_file);
  delete[] front_band.data;
  delete[] back_band.data;
}

// Search for runs of identical deltas, the so-called Run Length Encoding.
void BOS::compressBand(CompressedBand &band, int dem_id) {
  band.previous_delta = band.current_delta;
  band.current_delta = dem_id - band.previous_id;
  band.previous_id = dem_id;
  if (band.previous_delta != band.current_delta) {
    band.data[band.pointer] = band.delta_count;
    band.data[band.pointer + 1] = band.previous_delta;
    band.pointer += 2;
    band.delta_count = 0;
  }
  band.delta_count++;
}

void BOS::writeAndClose() {
  fseek(this->precomputed_data_file, this->dem.computable_points_count * 2 * 4, SEEK_SET);
  fwrite(&this->band_data_size, 4, 1, this->precomputed_data_file);
  fseek(this->precomputed_data_file, 0, SEEK_SET);
  fwrite(this->band_markers, 4, this->dem.computable_points_count * 2, this->precomputed_data_file);
  fclose(this->precomputed_data_file);
}

void BOS::loadBandData() {
  fread(this->band_markers, 4, this->dem.computable_points_count * 2, this->precomputed_data_file);
  fread(&this->band_data_size, 4, 1, this->precomputed_data_file);
  this->band_data = new short[this->band_data_size];
  fread(this->band_data, 2, this->band_data_size, this->precomputed_data_file);
  fclose(this->precomputed_data_file);
}

}  // namespace TVS
