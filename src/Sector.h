#include <string>
#include <chrono>

#include "DEM.h"
#include "BOS.h"
#include "Viewsheds.h"

#ifndef SECTOR_H
#define SECTOR_H

namespace TVS {

class Sector {
 public:
  DEM &dem;
  BOS bos_manager;
  Viewsheds viewsheds;

  int sector_angle;

  Sector(DEM&);
  void changeAngle(int);
  void sweepThroughAllBands();
  void prepareForCompute();
  void prepareForPreCompute();
  void calculateViewsheds();
};

}

#endif
