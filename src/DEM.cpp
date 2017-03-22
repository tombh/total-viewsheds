#include <sys/time.h>
#include <math.h>

#include <plog/Log.h>

#include "DEM.h"
#include "LinkedList.h"
#include "definitions.h"

namespace TVS {

DEM::DEM()
    : is_computing(!FLAGS_is_precompute),
      is_precomputing(FLAGS_is_precompute),
      width(FLAGS_dem_width),
      height(FLAGS_dem_height),
      size(FLAGS_dem_width * FLAGS_dem_height),
      sector_ordered(new int[size]),
      sight_ordered(new int[size]),
      distances(new float[size]),
      scale(FLAGS_dem_scale),
      max_los_as_points(FLAGS_max_line_of_sight / scale) {
  this->computable_points_count = 0;
  for (int point = 0; point < this->size; point++) {
    if (this->isPointComputable(point)) {
      this->computable_points_count++;
    }
  }
  this->tvs_width = sqrt(this->computable_points_count);
}

DEM::~DEM() {
  delete[] this->sector_ordered;
  delete[] this->sight_ordered;
  delete[] this->distances;
  if (this->is_computing) {
    delete[] this->elevations;
    delete[] this->tvs_complete;
  }
}

void DEM::prepareForCompute() {
  this->elevations = new float[this->size];
  this->tvs_complete = new float[this->computable_points_count];
  this->setElevations();
}

void DEM::setElevations() {
  FILE *f;
  unsigned short num;

  f = fopen(INPUT_DEM_FILE.c_str(), "rb");
  if (f == NULL) {
    LOG_ERROR << "Error opening: " << INPUT_DEM_FILE;
  } else {
    this->extractBTHeader(f);
    // The .bt DEM format starts at the bottom left and finishes
    // in the top right.
    int point, row_step, col_step;
    for (int x = 0; x < this->width; x++) {
      row_step = this->size + x;
      for (int y = 0; y < this->height; y++) {
        col_step = ((y + 1) * this->width);
        point = row_step - col_step;
        fread(&num, 2, 1, f);
        this->elevations[point] = (float)num;
      }
    }
  }
  fclose(f);
}

// Rather than getting into the whole gdal lib (which is amazing
// but a little complex) let's just hack the existing header for
// the TVS output.
// TODO: DRYer
void DEM::extractBTHeader(FILE *input_file) {
  FILE *output_file;
  unsigned char header[256];
  fread(&header, 256, 1, input_file);

  int cols = this->tvs_width;
  int rows = this->tvs_width;
  short tvs_point_size = 4;
  short is_floating_point = 1;

  double left_extent;
  memcpy(&left_extent, (header + 28), sizeof(double));
  left_extent += (double)FLAGS_max_line_of_sight;

  double right_extent;
  memcpy(&right_extent, (header + 36), sizeof(double));
  right_extent -= (double)FLAGS_max_line_of_sight;

  double bottom_extent;
  memcpy(&bottom_extent, (header + 44), sizeof(double));
  bottom_extent += (double)FLAGS_max_line_of_sight;

  double top_extent;
  memcpy(&top_extent, (header + 52), sizeof(double));
  top_extent -= (double)FLAGS_max_line_of_sight;

  output_file = fopen(TVS_RESULTS_FILE.c_str(), "wb");
  fwrite(&header, 256, 1, output_file);

  // Update header
  fseek(output_file, 10, SEEK_SET);
  fwrite(&cols, 4, 1, output_file);
  fseek(output_file, 14, SEEK_SET);
  fwrite(&rows, 4, 1, output_file);
  fseek(output_file, 18, SEEK_SET);
  fwrite(&tvs_point_size, 2, 1, output_file);
  fseek(output_file, 20, SEEK_SET);
  fwrite(&is_floating_point, 2, 1, output_file);

  fseek(output_file, 28, SEEK_SET);
  fwrite(&left_extent, 8, 1, output_file);
  fseek(output_file, 36, SEEK_SET);
  fwrite(&right_extent, 8, 1, output_file);
  fseek(output_file, 44, SEEK_SET);
  fwrite(&bottom_extent, 8, 1, output_file);
  fseek(output_file, 52, SEEK_SET);
  fwrite(&top_extent, 8, 1, output_file);

  fclose(output_file);
}

int DEM::povIdToTVSId(int pov_id) {
  int shifted_id =
    (((pov_id / this->width) - this->max_los_as_points) * this->tvs_width)
    + ((pov_id % this->width) - this->max_los_as_points);
  return shifted_id;
}

// Depending on the requested max line of sight, only certain points in the
// middle of the DEM can truly have their total visible surfaces calculated.
// This is because points on the edge do not have access to elevation data
// that may or not be visible.
bool DEM::isPointComputable(int dem_id) {
  int x = (dem_id % this->width) * FLAGS_dem_scale;
  int y = (dem_id / this->height) * FLAGS_dem_scale;
  int lower_x = FLAGS_max_line_of_sight;
  int upper_x = ((this->width - 1) * FLAGS_dem_scale) - FLAGS_max_line_of_sight;
  int lower_y = FLAGS_max_line_of_sight;
  int upper_y = ((this->height  - 1) * FLAGS_dem_scale) - FLAGS_max_line_of_sight;
  if (x >= lower_x && x <= upper_x && y >= lower_y && y <= upper_y) {
    return true;
  } else {
    return false;
  }
}

void DEM::setToPrecompute() {
  this->is_precomputing = true;
  this->is_computing = false;
}

void DEM::setToCompute() {
  this->prepareForCompute();
  this->is_precomputing = false;
  this->is_computing = true;
}

}  // namespace TVS
