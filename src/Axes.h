#include "DEM.h"

#ifndef AXES_H
#define AXES_H

namespace TVS {

class Axes {
 public:
  DEM &dem;

  Axes(DEM&);
  ~Axes();
  void adjust(int);
  void setDistancesFromVerticalAxis();
  void sortByDistanceFromHorizontalAxis();

 private:
  double *isin;
  double *icos;
  double *icot;
  double *itan;
  int *tmp1;
  int *tmp2;
  int quad;
  int sector_angle;
  double shift_angle;
  double computable_angle;

  void preComputeTrig();
  void preSort();
  void sort();
};
}

#endif
