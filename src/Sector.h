#include <string>

#include "LinkedList.h"
#include "DEM.h"

#ifndef SECTOR_H
#define SECTOR_H

namespace TVS {

class Sector {
 public:
  DEM &dem;

  LinkedList::node *nodes;
  LinkedList::node newnode;
  LinkedList::node newnode_trans;
  int *nodes_orth_ordered;

  LinkedList band_of_sight;
  int band_size;
  int half_band_size;

  FILE *precomputed_data_file;
  char *precomputed_data_path;
  char *ring_sector_data_path;
  bool is_store_ring_sectors;

  double scaled_observer_height;

  int sector_angle;
  double origin;
  double shift_angle;

  int **rsectorF;            // Rings sector rigth nodes
  int **rsectorB;            // Ring sector left nodes
  unsigned short *size_dsF;  // size number sector rigth
  unsigned short *size_dsB;  // size number sector rigth
  float *surfaceF;
  float *surfaceB;

  int quad;

  bool atright;
  bool atright_trans;

  int NEW_position;        // Position new node
  int NEW_position_trans;  // Auxliary new position node
  int PoV;                 // Position actuallly node

  // Sweep variables
  float open_delta_d;
  float delta_d;
  float open_delta_h;
  float delta_h;
  bool visible;
  float sur_dataF;
  float sur_dataB;
  float max_angle;
  int sweeppt;

  int rsF[1000][2];  // Temporal storage of visible ring sector up to 1000(large
                     // enough)
  int rsB[1000][2];  // Temporal storage of visible ring sector up to 1000(large
                     // enough)
  int nrsF;          // Number ring sector to right point
  int nrsB;          // Number ting sector to left point

  Sector(DEM&);  // Create Sector.
  ~Sector();             // dISPOSE Sector.

  void changeAngle();             // Change the Sector of 0 to 179

  // Gives each node its natural position and elevation
  void setHeights();

  void loopThroughBands(int);
  void post_loop(int);  // Operations after analysis

  void storers(int i);  // Storage dates of all ring sector this sector

  void calcule_pos_newnode(bool);
  void insert_node(LinkedList::node, int, bool);

  int presweepF();
  int presweepB();
  void sweepS(int i);  // Performs sweep node of sector (surface kernel);
  void kernelS(int rs[][2], int &nrs, float &sur_data);

  void init_storage();

  void closeprof(bool visible, bool fwd, bool vol, bool full);
  void recordsectorRS();  // Record dates rings sectors in files
  static void preComputedDataPath(char *, int);
  static void ringSectorDataPath(char *, int);
  void openPreComputedDataFile();
};

}  // namespace TVS

#endif
