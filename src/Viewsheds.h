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
  BOS &bos_manager;

  int sector_angle;
  int computable_band_size;
  int computable_bands;
  int total_band_points;
  int band_data_size;
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
  ClMem *band_markers;

  Viewsheds(DEM&, BOS&);
  static void ringDataPath(char*, int);
  static void threadedWrite(FILE*, int*, int, int);
  void calculate();
  void initialise();
  void transferDEMData();
  void transferSectorData();
  void transferToHost();
  void writeRingData();
};

}

#endif
