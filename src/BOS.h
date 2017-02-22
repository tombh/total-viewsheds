#include <stdio.h>
#include "DEM.h"
#include "LinkedList.h"

#ifndef BOS_H
#define BOS_H

namespace TVS {

class BOS {
 public:
  DEM &dem;

  int current_point;
  int *positions;
  int pov;
  int band_size;
  int half_band_size;
  bool remove;
  int sector_angle;
  LinkedList bos;
  LinkedList::node newnode;
  FILE *precomputed_data_file;

  BOS(DEM&);
  void setup(int);
  void adjustToNextPoint(int);
  void addNewNode();
  void setPointOfView();
  int calculateNewPosition();
  int getNewPosition();
  void insertNode();
  static void preComputedDataPath(char *, int);
  void openPreComputedDataFile();
  void writeAndClose();

 private:
  int ensureBandSizeIsOdd(int);
};
}

#endif
