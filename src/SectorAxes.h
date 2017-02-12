#include "DEM.h"
#include "Sector.h"

#ifndef SECTORAXES_H
#define SECTORAXES_H

namespace TVS {

class SectorAxes {
 public:
  Sector &sector;

  SectorAxes(Sector&);
  ~SectorAxes();
  void adjust();
  void setDistancesFromVerticalAxis();
  void sortByDistanceFromHorizontalAxis();

 private:
  double *isin;
  double *icos;
  double *icot;
  double *itan;
  int *tmp1;
  int *tmp2;
  double computable_angle;

  void preComputeTrig();
  void preSort();
  void sort();
};
}

#endif
