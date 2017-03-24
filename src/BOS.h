#include <stdio.h>
#include "DEM.h"
#include "LinkedList.h"

#ifndef BOS_H
#define BOS_H

namespace TVS {

class BOS {
 public:
  DEM &dem;

  struct CompressedBand {
   public:
    short current_delta;
    short previous_delta;
    short delta_count;
    int previous_id;
    short pointer;
    short *data;
  };

  int band_size;
  int half_band_size;
  int computable_band_size;
  int pov;
  int *band_markers;
  short *band_data;
  int band_data_size;
  int sector_angle;
  LinkedList bos;
  FILE *precomputed_data_file;

  BOS(DEM&);
  ~BOS();
  void initBandStorage();
  void setup(int);
  void adjustToAngle(int);
  void adjustToNextPoint(int);
  static void preComputedDataPath(char*, int);
  void buildComputableBands();
  void compressBand(CompressedBand&, int);
  void writeAndClose();
  void loadBandData();

 private:
  int new_point;
  bool remove;
  int sector_ordered_id;
  int dem_id;

  int ensureBandSizeIsOdd(int);
  void openPreComputedDataFile();
  int calculateNewPosition();
  int getNewPosition();
  void insertPoint();
};
}

#endif
