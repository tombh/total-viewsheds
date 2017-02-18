#include "DEM.h"
#include "Sector.h"

#ifndef COMPUTE_H
#define COMPUTE_H

namespace TVS {

class Compute {
 public:
  DEM dem;
  Sector sector;

  Compute();
  void forcePreCompute();
  void forceCompute();
  void run();
  void sumTVSSurfaces();
  void output();

 private:
  void initialize();
  void iterateSectors();
  void ensureDEMIsSquare();
};
}

#endif
