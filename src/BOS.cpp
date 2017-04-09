#include <numeric>

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
 * methods. Here we use a parallel band. Consider the following DEM,
 * where '/' represents the sides of a single BoS:
 *
 *    .  .  .  .  +  /  .  .  .
 *    .  .  .  /  +  .  .  .  .
 *    .  .  .  +  /  .  .  .  .
 *    .  .  /  +  .  .  .  .  .
 *    .  . (+) /  .  .  .  .  .
 *    .  /  +  .  .  .  .  .  .
 *    .  +  /  .  .  .  .  .  .
 *    /  +  .  .  .  .  .  .  .
 *    +  /  .  .  .  .  .  .  .
 *
 * The '(+)' is the viewer, or the Point of View (PoV) as it's referred
 * to here. The other '+'s are the points within the band of sight. In
 * effect there are actually 2 bands here, one from looking forward (the
 * front band) and another looking backwards (the back band).
 *
 * Even though BoSs are needed for every point (well every point that
 * doesn't have its line of sight truncated by the edge of the DEM) we
 * only need to calculate the *shape* of one BoS per sector angle. That
 * shape can then be applied to every point. The shape is described by
 * deltas describing the relative path from any given point. As far as I
 * know describing BoS shapes is a novel contribution to the state of the
 * art in viewshed analysis - I'd be very interested to know if you know
 * of any previous precendent.
 *
 * To create a single BoS from which to define the master shape for a
 * sector, we use the sorted DEM points from the `Axes` class. Using the
 * `sector_ordered` points we can populate the BoS but we don't know the
 * perpendicular distance each '+' point is from the PoV '(+)'. To then
 * order this specific subset of the DEM we use the `sight_ordered`
 * points.
**/
BOS::BOS(DEM &dem)
    : dem(dem),
      band_size((FLAGS_max_line_of_sight / FLAGS_dem_scale) + 1),
      band_deltas_size(band_size - 1),
      band_count(dem.computable_points_count * 2),
      total_band_points(band_count * band_size),
      band_deltas(new int[band_deltas_size]) {}

BOS::~BOS() { delete[] this->band_deltas; }

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

// Sort a 1D vector, but return the original indexes in their new order rather than
// the sorted values.
template <typename T>
std::vector<size_t> sort_indexes(const std::vector<T> &v) {
  // Initialize original index locations
  std::vector<size_t> idx(v.size());
  iota(idx.begin(), idx.end(), 0);
  // Sort indexes based on comparing values in v
  sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });
  return idx;
}

void BOS::compileBandData() {
  int current, next, dem_id;
  std::vector<int> band_distances, dem_ids;
  std::vector<size_t> sorted_indexes;

  // Increasing the band size is effectively like interpolating. The wider the band
  // the more points have the ability to block or count towards visibility for any
  // given line of sight. Adding a couple more points ensures that the band isn't
  // so thin that the line of sight 'slips' between DEM points, effectively
  // interpolating nothing.
  int band_samples = (this->band_size * 2) + 2;

  // Find the very middle of the DEM
  int pov_id = this->dem.size / 2;
  // We want the PoV to be in the middle of the band, both horizontally and
  // vertically.
  int band_edge = pov_id - (band_samples / 2);

  // Fill the band with half the points before the PoV and half after.
  // This stage just gives *which* points are in the band of sight, but not in
  // the correct order.
  for (int i = 0; i < band_samples; i++) {
    dem_id = this->dem.sector_ordered[band_edge + i];
    dem_ids.push_back(dem_id);
    // Make a note of the distance indexes (how far each point is from a line
    // orthogonal to the sector ordering).
    band_distances.push_back(this->dem.sight_ordered[dem_id]);
  }

  // These indexes will tell us which DEM IDs to pick so that the band of sight is
  // ordered in terms of points with increasing distance from the PoV.
  sorted_indexes = sort_indexes(band_distances);

  // Fill the actual band of sight with the deltas that trace a path from the PoV
  // to the final point in the band. These deltas apply to *all* possible bands in
  // this sector.
  int band_position = 0;
  int middle_of_whole_band = band_samples / 2;
  for (int i = middle_of_whole_band; i < (band_size * 2); i++) {
    current = dem_ids[sorted_indexes[i]];
    next = dem_ids[sorted_indexes[i + 1]];
    this->band_deltas[band_position] = current - next;
    band_position++;
  }
}

void BOS::saveBandData(int angle) {
  this->sector_angle = angle;
  this->openPreComputedDataFile();
  this->compileBandData();
  fwrite(this->band_deltas, 4, this->band_deltas_size, this->precomputed_data_file);
  fclose(this->precomputed_data_file);
}

void BOS::loadBandData(int angle) {
  this->sector_angle = angle;
  this->openPreComputedDataFile();
  fread(this->band_deltas, 4, this->band_deltas_size, this->precomputed_data_file);
  fclose(this->precomputed_data_file);
}

}  // namespace TVS
