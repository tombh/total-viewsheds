#include <plog/Log.h>

#include "Axes.h"
#include "BOS.h"
#include "Compute.h"
#include "DEM.h"
#include "Output.h"
#include "Viewsheds.h"
#include "definitions.h"
#include "helper.h"

namespace TVS {

Compute::Compute()
    : dem(DEM()),
      bos(BOS(dem)),
      viewsheds(Viewsheds(dem, bos)){
  this->ensureDEMIsSquare();
  helper::createDirectories();
}

void Compute::ensureDEMIsSquare() {
  if (dem.width != dem.height) {
    printf("Only square DEMs are currently supported.\n");
    exit(1);
  }
}

void Compute::preCompute() {
  LOG_INFO << "Starting precomputation.";
  this->dem.setToPrecompute();
  this->run();
}

void Compute::compute() {
  LOG_INFO << "Starting main computation.";
  this->dem.setToCompute();
  this->viewsheds.initialise();
  this->run();
}

void Compute::run() {
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

void Compute::changeAngle(int angle) {
  Axes axes = Axes(this->dem);
  axes.adjust(angle);
}

void Compute::iterateSectors() {
  // TODO: support non-integer angles and increments
  int increment = 180 / FLAGS_total_sectors;
  for (int angle = 0; angle < 180; angle += increment) {
    this->singleSector(angle);
  }
  if (this->dem.is_computing) {
    this->viewsheds.transferToHost();
  }
}

void Compute::singleSector(int angle) {
  LOGI << "Sector: " << angle;
  this->changeAngle(angle);
  if (this->dem.is_precomputing) {
    this->bos.saveBandData(angle);
  }
  if (this->dem.is_computing) {
    this->viewsheds.calculate(angle);
  }
}

void Compute::output() {
  LOG_INFO << "Writing final TVS data to " << TVS_RESULTS_FILE;
  Output output = Output(this->dem);
  output.tvsResults();
}

}  // namespace TVS
