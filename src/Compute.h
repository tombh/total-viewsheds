#include "BOS.h"
#include "DEM.h"
#include "Viewsheds.h"

#ifndef COMPUTE_H
#define COMPUTE_H

namespace TVS {

class Compute {
 public:
  DEM dem;
  BOS bos;
  Viewsheds viewsheds;

  Compute();
  void preCompute();
  void compute();
  void run();
  void output();
  void changeAngle(int);
  void iterateSectors();
  void singleSector(int);

 private:
  void ensureDEMIsSquare();
};
}

#endif
