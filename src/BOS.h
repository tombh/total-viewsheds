#include <stdio.h>
#include "DEM.h"
#ifndef BOS_H
#define BOS_H

namespace TVS {

class BOS {
 public:
  DEM &dem;

  int band_size;
  int band_deltas_size;
  int band_count;
  int total_band_points;
  int sector_angle;
  int *band_deltas;
  FILE *precomputed_data_file;

  BOS(DEM&);
  ~BOS();
  static void preComputedDataPath(char*, int);
  void compileBandData();
  void saveBandData(int);
  void loadBandData(int);
  void openPreComputedDataFile();
};
}

#endif
