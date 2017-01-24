#include <string>

#include "LinkedList.h"

#ifndef SECTOR_H
#define SECTOR_H

namespace TVS {

class Sector {
 public:
  FILE *precomputed_data_file;

  int sector_angle;

  LinkedList::node *nodes;         // Pointer nodes
  LinkedList::node newnode;        // New node of Linked List
  LinkedList::node newnode_trans;  // transitory node

  int bw;                    // Size windows sweep
  int hbw;                   // Size Windows sweep/2
  LinkedList band_of_sight;  // Linked list used

  bool is_store_ring_sectors;
  bool is_precomputing;

  char *precomputed_data_path;
  char *ring_sector_data_path;

  int *orderiAT;  // Pointers orderings

  double origin;                      // The angle windows sweep used
  double sect_angle;                  // The angle windows sweep of sector used
  double *isin, *icos, *icot, *itan;  // Trigonometric variable
  int *tmp1, *tmp2;                   // Temporal poninters

  int **rsectorF;            // Rings sector rigth nodes
  int **rsectorB;            // Ring sector left nodes
  unsigned short *size_dsF;  // size number sector rigth
  unsigned short *size_dsB;  // size number sector rigth
  float *surfaceF;
  float *surfaceB;

  int quad;  // Quadrant of maps

  double obs_h;  // Height at which an observer is

  bool foundn;        // Place to insert new node
  bool foundn_trans;  // Place to insert nes node
  bool foundd;        // Next dead
  bool foundd_trans;  // Next dad
  bool foundm;        // next main node
  bool atright;
  bool atright_trans;

  int NEW_position;        // Position new node
  int NEW_position_trans;  // Auxliary new position node
  int PoV;                 // Position actuallly node
  int NextPOV;             // Next position node Linked List

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
  // int sweep;
  int cnt_vec;

  int rsF[1000][2];  // Temporal storage of visible ring sector up to 1000(large
                     // enough)
  int rsB[1000][2];  // Temporal storage of visible ring sector up to 1000(large
                     // enough)
  int nrsF;          // Number ring sector to right point
  int nrsB;          // Number ting sector to left point

  Sector(bool = false);  // Create Sector.
  ~Sector();             // dISPOSE Sector.

  void pre_computed_sin();        // Calculate  the trigonometric variable sino
  void changeAngle();             // Change the Sector of 0 to 179
  void setDistances(int sector);  // Calculated distance from the axis starting

  // Gives each node its natural position and elevation
  void setHeights(double*);
  void presort();  // Ordering  based paper
  void sort();     // Ordering  based paper

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
