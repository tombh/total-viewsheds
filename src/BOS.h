#include <stdio.h>
#include "DEM.h"
#include "LinkedList.h"

#ifndef BOS_H
#define BOS_H

namespace TVS {

class BOS {
 public:
  DEM &dem;

  int current_point;
  int pov;
  int band_size;
  int half_band_size;
  LinkedList bos;
  LinkedList::node newnode;
  LinkedList::node newnode_trans;
  FILE *precomputed_data_file;

  bool atright;
  bool atright_trans;
  int NEW_position;
  int NEW_position_trans;

  BOS(DEM&);
  ~BOS();
  void setup(FILE*);
  void setPointOfView();
  void adjustToNextPoint(int);

 private:
  void calculateNewnodePosition(bool);
  void insertNode(LinkedList::node, int, bool);
};
}

#endif
