#include "Sector.h"
#include "definitions.h"

#ifndef DEM_H
#define DEM_H

namespace TVS {

class DEM {
  struct color;

 public:
  double *heights;
  bool is_store_ring_sectors;

  int width;
  int height;
  int size;
  int scale;
  int max_viewshed;
  float tvs[DEM_SIZE];
  float cumulative_surface[DEM_SIZE];

  explicit DEM();

  void preCompute();
  void compute(bool = false);

 private:
  Sector sector;
  double dtime();
  double *readHeights();
};
}

#endif
