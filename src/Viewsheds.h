#include <stdio.h>

#include <arrayfire.h>

#include "BOS.h"
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

  af::timer mark;
  af::array elevations;
  af::array distances;
  af::array distance_deltas;
  af::array bands;
  af::array angles;
  af::array visible;
  af::array opens;
  af::array closes;
  af::array cumulative_surfaces;

  Viewsheds(DEM&);
  static void ringSectorDataPath(char *, int);
  void calculate(int);
  void timerStart();
  void timerLog(const char);
  void initialise();
  void transferDEMData();
  void transferBandData(BOS&);
  void transferSectorData(BOS&);
  void transferToHost();
  void hullPass();
  void findRingSectors();
  void findVisiblePoints();
  void sumSurfaces();
};

}

#endif
