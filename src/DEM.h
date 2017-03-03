#include <stdio.h>
#include "LinkedList.h"

#ifndef DEM_H
#define DEM_H

namespace TVS {

class DEM {
 public:
  int width;
  int height;
  int size;
  int scale;
  float scaled_observer_height;
  int computable_points_count;
  int tvs_width;

  int *sector_ordered;
  int *sight_ordered;
  float *distances;
  float *elevations;
  float *tvs_complete;

  bool is_computing;
  bool is_precomputing;

  DEM();
  ~DEM();
  void setElevations();
  void extractBTHeader(FILE*);
  bool isPointComputable(int);
  void prepareForCompute();
  void setToPrecompute();
  void setToCompute();

};
}

#endif
