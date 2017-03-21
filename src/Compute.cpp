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
    this->sector.prepareForCompute();
  } else {
    this->sector.prepareForPreCompute();
    LOG_INFO << "Starting precomputation.";
  }

  if (FLAGS_single_sector != -1) {
    this->singleSector(FLAGS_single_sector);
  } else {
    this->iterateSectors();
  }

  if (this->dem.is_computing) {
    LOG_INFO << "Finished main computation.";
  } else {
    LOG_INFO << "Finished precomputation.";
  }
}

void Compute::iterateSectors() {
  // TODO: support non-integer angles and increments
  int increment = 180 / FLAGS_total_sectors;
  for (int angle = 0; angle < 180; angle += increment) {
    this->singleSector(angle);
  }
  if (this->dem.is_computing) {
    this->sector.viewsheds.transferToHost();
  }
}

void Compute::singleSector(int angle) {
  LOGI << "Sector: " << angle;
  this->sector.changeAngle(angle);
  if (this->dem.is_precomputing) {
    this->sector.sweepThroughAllBands();
  }
  if (this->dem.is_computing) {
    this->sector.calculateViewsheds();
  }
}

void Compute::output() {
  LOG_INFO << "Writing final TVS data to " << TVS_RESULTS_FILE;
  Output output = Output(this->dem);
  output.tvsResults();
}

}  // namespace TVS
