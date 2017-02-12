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

float DEM::maxViewshedValue() {
  float current_max = 0;
  for (int point = 0; point < this->size; point++) {
    if (this->cumulative_surface[point] > current_max) {
      current_max = this->cumulative_surface[point];
    }
  }
  return current_max;
}

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
