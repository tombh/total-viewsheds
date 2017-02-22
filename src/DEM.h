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
  LinkedList::node *nodes;
  int *nodes_sector_ordered;
  float *cumulative_surface;

  bool is_computing;
  bool is_precomputing;

  DEM();
  ~DEM();
  void setHeights();
  void setNodeIDs();
  void extractBTHeader(FILE*);
  float maxViewshedValue();
  float minViewshedValue();
  void prepareForCompute();
  void setToPrecompute();
  void setToCompute();

};
}

#endif
