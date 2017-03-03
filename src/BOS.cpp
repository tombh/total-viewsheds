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
**/

BOS::BOS(DEM &dem)
    : dem(dem),
      // Size of the Band of Sight.
      band_size(ensureBandSizeIsOdd(FLAGS_dem_width)),
      // The half band helps decide which points to add next.
      half_band_size((band_size - 1) / 2),
      // Limit and standardise final viewshed computations. The limit
      // is used to restrict the area search for the viewshed. Whereas
      // having a consistent size of band is usefel for vectorised
      // parallel computation.
      computable_band_size((FLAGS_max_line_of_sight / FLAGS_dem_scale) + 1),
      // The actual Band of Sight data structure.
      bos(LinkedList(band_size)) {}

BOS::~BOS() {
  if (this->dem.is_computing) this->deleteBandStorage();
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
    LOGI << "Raising `band_size` from " << requested_band_size - 1 << " to "
         << requested_band_size;
  }
  return requested_band_size;
}

void BOS::openPreComputedDataFile() {
  const char *mode;
  char path[100];
  this->preComputedDataPath(path, this->sector_angle);
  if (this->dem.is_precomputing) {
    mode = "wb";
  } else {
    mode = "rb";
  }
  this->precomputed_data_file = fopen(path, mode);
  if (this->precomputed_data_file == NULL) {
    LOG_ERROR << "Couldn't open " << path
              << " for sector_angle: " << this->sector_angle;
    throw "Couldn't open file.";
  }
}

void BOS::preComputedDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", SECTOR_DIR.c_str(), sector_angle);
}

// Set the band of sight up for the very first time, namely after a sector angle
// change.
void BOS::setup(int sector_angle) {
  this->positions = new int[this->dem.size];
  this->sector_angle = sector_angle;
  this->sector_ordered_id = 0;
  this->openPreComputedDataFile();
  if(this->dem.is_computing) {
    this->current_band = -1;
    this->initBandStorage();
    fread(this->positions, 4, this->dem.size, this->precomputed_data_file);
  }
  this->pov = 0;
  this->bos.Clear();
  this->bos.FirstNode(this->dem.sector_ordered[0]);
}

void BOS::initBandStorage() {
  this->bands = new int[this->dem.computable_points_count * this->computable_band_size * 2];
}

void BOS::deleteBandStorage() {
  delete[] this->bands;
}

void BOS::writeAndClose() {
  if (this->dem.is_precomputing && this->sector_ordered_id == this->dem.size - 1) {
    fwrite(this->positions, 4, this->dem.size, this->precomputed_data_file);
  }
  fclose(this->precomputed_data_file);
  delete[] this->positions;
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

// Where does the new node fit in the current BoS?
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
        LOGE << "new_point sight_ordered index: " << new_point
             << " is bigger than anything in band of sight.";
        throw;
      }
    }
    position = this->bos.LL[sweep].prev;
  }
  return position;
}

int BOS::getNewPosition() {
  int position;
  if (this->dem.is_computing) {
    position = this->positions[this->new_point];
  }
  if (this->dem.is_precomputing) {
    position = this->calculateNewPosition();
    this->positions[this->new_point] = position;
  }
  return position;
}

void BOS::insertPoint() {
  int position = this->getNewPosition();
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

void BOS::buildBands(int dem_id) {
  this->dem_id = dem_id;
  if (this->dem.isPointComputable(dem_id)) {
    this->sweepForward();
    this->sweepBackward();
  }
}

void BOS::sweepForward() {
  this->current_band++;
  int section = this->current_band * this->computable_band_size;
  this->bands[section] = this->bos.LL[this->pov].dem_id;

  int count = 1;
  int sweep = this->bos.LL[this->pov].next;
  while (count < this->computable_band_size) {
    this->bands[section + count] = this->bos.LL[sweep].dem_id;
    sweep = this->bos.LL[sweep].next;
    count++;
  }
}

void BOS::sweepBackward() {
  this->current_band++;
  int section = this->current_band * this->computable_band_size;
  this->bands[section] = this->bos.LL[this->pov].dem_id;

  int count = 1;
  int sweep = this->bos.LL[this->pov].prev;
  while (count < this->computable_band_size) {
    this->bands[section + count] = this->bos.LL[sweep].dem_id;
    sweep = this->bos.LL[sweep].prev;
    count++;
  }
}

}  // namespace TVS
