#include <string>
#include <chrono>

#include "DEM.h"
#include "BOS.h"

#ifndef SECTOR_H
#define SECTOR_H

namespace TVS {

class Sector {
 public:
  DEM &dem;
  BOS bos_manager;

  int point;

  FILE *precomputed_data_file;
  char *precomputed_data_path;
  char *ring_sector_data_path;
  bool is_store_ring_sectors;

  double scaled_observer_height;

  int sector_angle;

  int **rsectorF;
  int **rsectorB;
  unsigned short *size_dsF;
  unsigned short *size_dsB;
  float *surfaceF;
  float *surfaceB;

  int quad;
  float h;
  float d;
  float open_delta_d;
  float delta_d;
  float open_delta_h;
  float delta_h;
  bool visible;
  float sur_dataF;
  float sur_dataB;
  float max_angle;
  int sweeppt;
  int rsF[1000][2];
  int rsB[1000][2];
  int nrsF;
  int nrsB;

  std::chrono::high_resolution_clock::time_point timestamp;

  Sector(DEM&);
  ~Sector();

  void initialize();
  void changeAngle(int);
  void setHeights();
  void extractBTHeader(FILE *);
  void loopThroughBands();
  void storers(int i);

  int presweepF();
  int presweepB();
  void progressUpdate();
  void sweepS();
  void sweepInit();
  void sweepForward();
  void sweepBackward();
  void kernelS(int rs[][2], int &nrs, float &sur_data);

  void closeprof(bool visible, bool fwd);
  void recordsectorRS();
  static void preComputedDataPath(char *, int);
  static void ringSectorDataPath(char *, int);
  void openPreComputedDataFile();
};

}

#endif
