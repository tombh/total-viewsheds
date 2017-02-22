#include <sys/time.h>

#include <plog/Log.h>

#include "definitions.h"
#include "LinkedList.h"
#include "DEM.h"

namespace TVS {

DEM::DEM()
    : is_computing(!FLAGS_is_precompute),
      is_precomputing(FLAGS_is_precompute),
      size(FLAGS_dem_width * FLAGS_dem_height),
      nodes(new LinkedList::node[size]),
      nodes_sector_ordered(new int[size]),
      cumulative_surface(new float[size]),
      width(FLAGS_dem_width),
      height(FLAGS_dem_height),
      scale(FLAGS_dem_scale) {
}

DEM::~DEM() {
  delete[] this->nodes;
  delete[] this->nodes_sector_ordered;
  delete[] this->cumulative_surface;
}

void DEM::setHeights() {
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
        this->nodes[point].idx = point;

        fread(&num, 2, 1, f);

        // Why divide by SCALE?
        // Surely this will effect spherical earth based calculations?
        this->nodes[point].h = (float)num / this->scale;  // decameters
      }
    }
  }
  fclose(f);
}

void DEM::setNodeIDs() {
  for (int point = 0; point < this->size; point++) {
    this->nodes[point].idx = point;
  }
}

// Rather than getting into the whole gdal lib (which is amazing
// but a little complex) let's just hack the existing header for
// the TVS output.
void DEM::extractBTHeader(FILE *input_file) {
  FILE *output_file;
  unsigned char header[256];
  short tvs_point_size = 4;
  short is_floating_point = 1;

  fread(&header, 256, 1, input_file);
  output_file = fopen(TVS_RESULTS_FILE.c_str(), "wb");
  fwrite(&header, 256, 1, output_file);

  // Update header for floating point TVS values
  fseek(output_file, 18, SEEK_SET);
  fwrite(&tvs_point_size, 2, 1, output_file);
  fseek(output_file, 20, SEEK_SET);
  fwrite(&is_floating_point, 2, 1, output_file);

  fclose(output_file);
}

// TODO: Combine with min
float DEM::maxViewshedValue() {
  float current_max = 0;
  for (int point = 0; point < this->size; point++) {
    if (this->cumulative_surface[point] > current_max) {
      current_max = this->cumulative_surface[point];
    }
  }
  return current_max;
}

// TODO: combine with max
float DEM::minViewshedValue() {
  float current_min = this->cumulative_surface[0];
  for (int point = 1; point < this->size; point++) {
    if (this->cumulative_surface[point] < current_min) {
      current_min = this->cumulative_surface[point];
    }
  }
  return current_min;
}

void DEM::setToPrecompute() {
  this->is_precomputing = true;
  this->is_computing = false;
}

void DEM::setToCompute() {
  this->is_precomputing = false;
  this->is_computing = true;
}

void DEM::prepareForCompute() {
  for (int point = 0; point < this->size; point++) {
    this->cumulative_surface[point] = 0;
  }
}

}  // namespace TVS
