#include <stdio.h>
#include <thread>

#include "BOS.h"
#include "Clamp.h"
#include "DEM.h"

#ifndef VIEWSHEDSONGPU_H
#define VIEWSHEDSONGPU_H

namespace TVS {

class Viewsheds {
 public:
  DEM &dem;
  BOS &bos;

  int sector_angle;
  int ring_data_size;
  int reserved_rings;
  int computed_sectors;
  int batches;
  std::thread *ring_writer_threads;

  Clamp cl;
  ClDev *gpu;
  ClProgram *program;
  ClKernel *kernel;
  ClMem *cumulative_surfaces;
  ClMem *sector_rings;
  ClMem *distances;
  ClMem *elevations;
  ClMem *band_deltas;

  Viewsheds(DEM &, BOS &);
  static void ringDataPath(char *, int);
  static void threadedWrite(FILE *, int *, int, int);
  void removeRingData();
  void initialise();
  void prepareMemory();
  void prepareKernel();
  void calculate(int);
  void transferSectorData();
  void transferToHost();
  void writeRingData();
};
}

#endif
