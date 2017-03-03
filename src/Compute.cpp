#include <sys/time.h>
#include <arrayfire.h>

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
    this->dem.setElevations();
    this->sector.prepareForCompute();
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
  // TODO: support non-integer angles and increments
  int increment = 180 / FLAGS_total_sectors;
  for (int angle = 0; angle < 180; angle += increment) {

    char *time = NULL;
    af::sync();
    af::timer mark = af::timer::start();

    LOGD << "Sector: " << angle;
    this->sector.changeAngle(angle);
    this->sector.loopThroughBands();
    if (this->dem.is_computing) {
      this->sector.calculateViewsheds();
    }

    af::sync();
    asprintf(&time, "Sector time: %g", af::timer::stop(mark));
    LOGI << time;
    free(time);
  }

  if (this->dem.is_computing) {
    this->sector.viewsheds.transferToHost();
  }
}

void Compute::output() {
  LOG_INFO << "Writing final TVS data to " << TVS_RESULTS_FILE;
  Output output = Output(this->dem);
  output.tvsResults();
}

}  // namespace TVS
