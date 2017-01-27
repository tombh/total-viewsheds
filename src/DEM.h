#ifndef DEM_H
#define DEM_H

namespace TVS {

class DEM {
 public:
  bool is_store_ring_sectors;
  bool is_computing;
  bool is_precomputing;
  int width;
  int height;
  int size;
  int scale;
  float *cumulative_surface;

  DEM();
  ~DEM();
  void writeTVSOutput();
  float maxViewshedValue();
  float minViewshedValue();
  void compute();

 private:
  double dtime();
};
}

#endif
