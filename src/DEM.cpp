#include <sys/time.h>

#include <plog/Log.h>

#include "definitions.h"
#include "helper.h"
#include "DEM.h"
#include "Output.h"
#include "Sector.h"

namespace TVS {

DEM::DEM()
    : size(FLAGS_dem_width * FLAGS_dem_height),
      cumulative_surface(new float[size]),
      width(FLAGS_dem_width),
      height(FLAGS_dem_height),
      scale(FLAGS_dem_scale) {
}

DEM::~DEM() {
  delete[] this->cumulative_surface;
}

double DEM::dtime() {
  double tseconds = 0.0;
  struct timeval mytime;
  gettimeofday(&mytime, (struct timezone *)0);
  tseconds = (double)(mytime.tv_sec + (double)mytime.tv_usec * 1.0e-6);
  return (tseconds);
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

void DEM::compute() {
  double timer0, timer1, timer2, timer3, timer4;

  this->is_precomputing = FLAGS_is_precompute;
  this->is_computing = !FLAGS_is_precompute;

  helper::createDirectories();

  Sector sector = Sector(*this);
  sector.init_storage();

  if (is_computing) {
    LOG_INFO << "Starting main computation.";
    for (int point = 0; point < this->size; point++) {
      this->cumulative_surface[point] = 0;
    }
    sector.setHeights();
  } else {
    LOG_INFO << "Starting precomputation.";
  }

  timer0 = this->dtime();

  for (int angle = 0; angle < FLAGS_total_sectors; angle++) {
    LOGD << "Sector: " << angle;
    timer1 = dtime();
    sector.loopThroughBands(angle);
    timer2 = dtime();
    if (is_computing) {
      if (FLAGS_is_store_ring_sectors) sector.recordsectorRS();
      for (int point = 0; point < this->size; point++) {
        this->cumulative_surface[point] += sector.surfaceF[point] + sector.surfaceB[point];
      }
    }
  }

  if (is_computing) {
    LOG_INFO << "Finished main computation.";
  } else {
    LOG_INFO << "Finished  precomputation.";
  }
}

void DEM::writeTVSOutput() {
  Output output = Output(*this);
  output.tvsResults();
}

}  // namespace TVS
