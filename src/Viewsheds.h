#include <thread>
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
  int ring_data_size;
  int reserved_rings;
  int computed_sectors;
  std::thread *ring_writer_threads;

  Clamp cl;
  ClDev *gpu;
  ClProgram *program;
  ClKernel *kernel;
  ClMem *cumulative_surfaces;
  ClMem *sector_rings;
  ClMem *distances;
  ClMem *elevations;
  ClMem *bands;

  Viewsheds(DEM&);
  static void ringDataPath(char*, int);
  void calculate(int);
  void initialise();
  void transferDEMData();
  void transferSectorData(BOS&);
  void transferToHost();
  void writeRingData(int*, int);
};

}

#endif
