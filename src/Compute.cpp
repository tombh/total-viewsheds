#include <sys/time.h>

#include <plog/Log.h>

#include "Compute.h"
#include "DEM.h"
#include "Output.h"
#include "Sector.h"
#include "definitions.h"
#include "helper.h"

namespace TVS {

Compute::Compute()
    : dem(DEM()),
      sector(Sector(dem)) {}

void Compute::initialize() {
  this->ensureDEMIsSquare();
  helper::createDirectories();
  this->sector.initialize();
}

void Compute::ensureDEMIsSquare() {
  if (dem.width != dem.height) {
    printf("Only square DEMs are currently supported.\n");
    exit(1);
  }
}

void Compute::forcePreCompute() {
  this->dem.setToPrecompute();
  this->run();
}

void Compute::forceCompute() {
  this->dem.setToCompute();
  this->run();
}

void Compute::run() {
  this->initialize();

  if (this->dem.is_computing) {
    LOG_INFO << "Starting main computation.";
    this->dem.prepareForCompute();
    this->sector.setHeights();
  } else {
    LOG_INFO << "Starting precomputation.";
  }

  this->iterateSectors();

  if (this->dem.is_computing) {
    LOG_INFO << "Finished main computation.";
  } else {
    LOG_INFO << "Finished precomputation.";
  }
}

void Compute::iterateSectors() {
  // TODO: iterate by 180/FLAGS_total_sectors
  for (int angle = 0; angle < FLAGS_total_sectors; angle++) {
    LOGD << "Sector: " << angle;
    this->sector.changeAngle(angle);
    this->sector.loopThroughBands();
    if (this->dem.is_computing) {
      this->sumTVSSurfaces();
      if (FLAGS_is_store_ring_sectors) this->sector.recordsectorRS();
    }
  }
}

// For every sector change we need to keep track of the new visible surface
// areas that have been discovered.
void Compute::sumTVSSurfaces() {
  double f_plus_b;
  for (int point = 0; point < this->dem.size; point++) {
    f_plus_b = this->sector.surfaceF[point] + this->sector.surfaceB[point];
    this->dem.cumulative_surface[point] += f_plus_b;
  }
}

void Compute::output() {
  Output output = Output(this->dem);
  output.tvsResults();
  output.tvsToPNG();
}

}  // namespace TVS
