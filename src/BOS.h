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
  int pov;
  int *bands;
  int current_band;
  int sector_angle;
  LinkedList bos;
  FILE *precomputed_data_file;

  BOS(DEM&);
  ~BOS();
  void setup(int);
  void adjustToNextPoint(int);
  static void preComputedDataPath(char *, int);
  void buildBands(int);
  void sweepForward();
  void sweepBackward();
  void writeAndClose();

 private:
  int *positions;
  int new_point;
  bool remove;
  int sector_ordered_id;
  int dem_id;

  int ensureBandSizeIsOdd(int);
  void openPreComputedDataFile();
  void initBandStorage();
  void deleteBandStorage();
  void addNewNode();
  int calculateNewPosition();
  int getNewPosition();
  void insertPoint();
  void prefillBands();

};
}

#endif
