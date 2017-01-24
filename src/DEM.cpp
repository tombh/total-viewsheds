#include <sys/time.h>

#include <plog/Log.h>

#include "DEM.h"
#include "Output.h"
#include "Sector.h"
#include "definitions.h"

namespace TVS {

DEM::DEM() : size(DEM_SIZE), width(DEM_WIDTH), height(DEM_HEIGHT) {}

double DEM::dtime() {
  double tseconds = 0.0;
  struct timeval mytime;
  gettimeofday(&mytime, (struct timezone *)0);
  tseconds = (double)(mytime.tv_sec + (double)mytime.tv_usec * 1.0e-6);
  return (tseconds);
}

double *DEM::readHeights() {
  FILE *f;
  unsigned short num;
  double *h = new double[DEM_SIZE * sizeof(double)];
  f = fopen(INPUT_DEM_FILE, "rb");
  if (f == NULL) {
    LOG_ERROR << "Error opening: " << INPUT_DEM_FILE;
  } else {
    for (int i = 0; i < DEM_SIZE; i++) {
      fread(&num, 2, 1, f);

      // Original reading does this funny geometric translation...
      // It's like the DEM is a piece of paper and you flip the top-right corner
      // to the bottom-left. But why!?
      // h[i/xdim+ydim*(i%xdim)] = num/SCALE; //internal representation from top
      // to row.

      // Why divide by SCALE? Is it multiplied in again later for the total
      // viewshed surface area calculation?
      h[i] = (float)num / DEM_SCALE;
    }
  }
  return h;
}

void DEM::preCompute() { this->compute(true); }

void DEM::compute(bool is_precomputing) {
  double timer0, timer1, timer2, timer3, timer4;

  // this->sector = Sector(is_precomputing);
  this->sector.is_precomputing = is_precomputing;
  this->sector.init_storage();

  if (!is_precomputing) {
    LOG_INFO << "Starting main computation.";
    this->cumulative_surface[this->size];
    for (int point = 0; point < this->size; point++)
      this->cumulative_surface[point] = 0;
    this->heights = this->readHeights();
    this->sector.setHeights(this->heights);
  } else {
    LOG_INFO << "Starting precomputation.";
  }

  timer0 = this->dtime();

  for (int sector_angle = 0; sector_angle < TOTAL_SECTORS; sector_angle++) {
    LOGD << "Sector: " << sector_angle;
    timer1 = dtime();
    this->sector.loopThroughBands(sector_angle);
    timer2 = dtime();
    if (!is_precomputing) {
      if (IS_STORE_RING_SECTORS) this->sector.recordsectorRS();
      for (int point = 0; point < this->size; point++) {
        this->cumulative_surface[point] +=
            this->sector.surfaceF[point] + this->sector.surfaceB[point];
      }
    }
  }

  if (!is_precomputing) {
    LOG_INFO << "Finished main computation.";
    Output output = Output(*this);
    output.tvsResults();
  } else {
    LOG_INFO << "Finished  precomputation.";
  }
}

}  // namespace TVS
