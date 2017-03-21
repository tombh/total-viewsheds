#include <stdio.h>
#include "DEM.h"
#include "LinkedList.h"

#ifndef BOS_H
#define BOS_H

namespace TVS {

class BOS {
 public:
  DEM &dem;

  int band_size;
  int half_band_size;
  int computable_band_size;
  int total_band_points;
  int pov;
  int *bands;
  int current_band;
  int sector_angle;
  LinkedList bos;
  FILE *precomputed_data_file;

  BOS(DEM&);
  ~BOS();
  void initBandStorage();
  void setup(int);
  void adjustToNextPoint(int);
  static void preComputedDataPath(char *, int);
  void buildComputableBands();
  void writeAndClose();

 private:
  int new_point;
  bool remove;
  int sector_ordered_id;
  int dem_id;

  int ensureBandSizeIsOdd(int);
  void openPreComputedDataFile();
  void addNewNode();
  int calculateNewPosition();
  int getNewPosition();
  void insertPoint();
};
}

#endif
