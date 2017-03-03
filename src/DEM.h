#include <stdio.h>
#include "LinkedList.h"

#ifndef DEM_H
#define DEM_H

namespace TVS {

class DEM {
 public:
  bool is_computing;
  bool is_precomputing;
  int width;
  int height;
  int size;
  int *sector_ordered;
  int *sight_ordered;
  float *elevations;
  float *distances;
  int scale;
  float *tvs_complete;
  float scaled_observer_height;
  int computable_points_count;
  int tvs_width;

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
