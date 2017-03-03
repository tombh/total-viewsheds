#include <stdio.h>

#include "BOS.h"
#include "Clamp.h"
#include "DEM.h"

#ifndef VIEWSHEDSONGPU_H
#define VIEWSHEDSONGPU_H

namespace TVS {

class Viewsheds {
 public:
  DEM &dem;

  int sector_angle;
  int computable_band_size;
  int computable_bands;
  int total_band_points;
  int tvs_size;
  Clamp cl;
  ClDev *gpu;
  ClProgram *program;
  ClKernel *kernel;
  ClMem *cumulative_surfaces;
  ClMem *distances;
  ClMem *elevations;
  ClMem *bands;

  Viewsheds(DEM&);
  static void ringSectorDataPath(char *, int);
  void calculate(int);
  void initialise();
  void transferDEMData();
  void transferSectorData(BOS&);
  void transferToHost();
};

}

#endif
