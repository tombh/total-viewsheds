#include <math.h>
#include <chrono>

#include <plog/Log.h>

#include "Axes.h"
#include "BOS.h"
#include "DEM.h"
#include "Sector.h"
#include "definitions.h"

namespace TVS {

Sector::Sector(DEM &dem)
    : dem(dem),
      // Scaling all measurements to a single unit of the DEM_SCALE makes
      // calculations simpler.
      // TODO: I think this could be a problem when considering DEM's without
      // curved earth projections?
      // TODO: This is probably better associated as a property of the DEM
      scaled_observer_height(FLAGS_observer_height / dem.scale),
      bos_manager(BOS(dem)) {}

void Sector::initialize() {
  this->timestamp = std::chrono::high_resolution_clock::now();
  if (this->dem.is_computing) {
    this->surfaceF = new float[this->dem.size];
    this->surfaceB = new float[this->dem.size];

    // TODO: sometimes surfaceB doesn't get any surface for
    // some points. Is this reasonable or a bug, perhaps in
    // absent nodes from the band of sight?
    // So for now, just initialise all items to 0
    for (int point = 0; point < this->dem.size; point++) {
      this->surfaceF[point] = 0;
      this->surfaceB[point] = 0;
    }

    if (FLAGS_is_store_ring_sectors) {
      this->rsectorF = new int *[this->dem.size];
      this->rsectorB = new int *[this->dem.size];
      this->size_dsF = new unsigned short[this->dem.size];
      this->size_dsB = new unsigned short[this->dem.size];
    }
  }
}

Sector::~Sector() {
  if (this->dem.is_computing) {
    delete[] this->surfaceF;
    delete[] this->surfaceB;
    if (FLAGS_is_store_ring_sectors) {
      delete[] this->rsectorF;
      delete[] this->rsectorB;
      delete[] this->size_dsF;
      delete[] this->size_dsB;
    }
  }
}

void Sector::loopThroughBands() {
  this->openPreComputedDataFile();
  this->bos_manager.setup(this->precomputed_data_file);
  for (int point = 0; point < this->dem.size; point++) {
    this->bos_manager.adjustToNextPoint(point);
    if (this->dem.is_computing) {
      this->sweepS();
      this->storers(this->dem.nodes_sector_ordered[point]);
    }
    this->progressUpdate();
  }
  fclose(this->precomputed_data_file);
}

void Sector::changeAngle(int sector_angle) {
  LOGD << "Changing sector angle to: " << sector_angle;
  this->sector_angle = sector_angle;
  Axes axes = Axes(this->dem);
  axes.adjust(sector_angle);
}

// TODO: This should be in DEM
void Sector::setHeights() {
  FILE *f;
  unsigned short num;
  f = fopen(INPUT_DEM_FILE.c_str(), "rb");
  if (f == NULL) {
    LOG_ERROR << "Error opening: " << INPUT_DEM_FILE;
  } else {
    this->extractBTHeader(f);
    // The .bt DEM format starts at the bottom left and finishes
    // in the top right.
    int point, row_step, col_step;
    for (int x = 0; x < this->dem.width; x++) {
      row_step = this->dem.size + x;
      for (int y = 0; y < this->dem.height; y++) {
        col_step = ((y + 1) * this->dem.width);
        point = row_step - col_step;
        this->dem.nodes[point].idx = point;

        fread(&num, 2, 1, f);

        // Why divide by SCALE?
        // Surely this will effect spherical earth based calculations?
        this->dem.nodes[point].h = (float)num / this->dem.scale;  // decameters
      }
    }
  }
  fclose(f);
}

// Rather than getting into the whole gdal lib (which is amazing
// but a little complex) let's just hack the existing header for
// the TVS output.
void Sector::extractBTHeader(FILE *input_file) {
  FILE *output_file;
  unsigned char header[256];
  short tvs_point_size = 4;
  short is_floating_point = 1;

  fread(&header, 256, 1, input_file);
  output_file = fopen(TVS_RESULTS_FILE.c_str(), "wb");
  fwrite(&header, 256, 1, output_file);

  // Update header for floating point TVS values
  fseek(output_file, 18, SEEK_SET);
  fwrite(&tvs_point_size, 2, 1, output_file);
  fseek(output_file, 20, SEEK_SET);
  fwrite(&is_floating_point, 2, 1, output_file);

  fclose(output_file);
}

// TODO: This should be in BOS
void Sector::openPreComputedDataFile() {
  const char *mode;
  char path[100];
  this->preComputedDataPath(path, this->sector_angle);
  if (this->dem.is_precomputing) {
    mode = "wb";
  } else {
    mode = "rb";
  }
  this->precomputed_data_file = fopen(path, mode);
  if (this->precomputed_data_file == NULL) {
    LOG_ERROR << "Couldn't open " << path
              << " for sector_angle: " << this->sector_angle;
    throw "Couldn't open file.";
  }
}

// Fulldata to store ring sector topology
void Sector::storers(int i) {
  float surscale =
      PI_F / (360 * this->dem.scale * this->dem.scale);  // hectometers^2
  this->surfaceF[i] = this->sur_dataF * surscale;
  this->surfaceB[i] = this->sur_dataB * surscale;

  if (FLAGS_is_store_ring_sectors) {
    this->rsectorF[i] = new int[this->nrsF * 2];
    this->rsectorB[i] = new int[this->nrsB * 2];
    this->size_dsF[i] = 2 * this->nrsF;
    this->size_dsB[i] = 2 * this->nrsB;
    for (int j = 0; j < this->nrsF; j++) {
      this->rsectorF[i][2 * j + 0] = this->rsF[j][0];
      this->rsectorF[i][2 * j + 1] = this->rsF[j][1];
    }
    for (int j = 0; j < nrsB; j++) {
      this->rsectorB[i][2 * j + 0] = this->rsB[j][0];
      this->rsectorB[i][2 * j + 1] = this->rsB[j][1];
    }
  }
}

// Ring sectors (along with sector angle) contain all the information
// to recreate full viewshed data.
void Sector::recordsectorRS() {
  FILE *fs;
  char path_ptr[100];
  int no_of_ring_sectors;
  this->ringSectorDataPath(path_ptr, this->sector_angle);
  fs = fopen(path_ptr, "wb");

  this->rsectorF[dem.nodes_sector_ordered[this->dem.size - 1]] = new int[0];
  this->rsectorB[dem.nodes_sector_ordered[this->dem.size - 1]] = new int[0];
  this->size_dsB[dem.nodes_sector_ordered[this->dem.size - 1]] = 0;
  this->size_dsF[dem.nodes_sector_ordered[this->dem.size - 1]] = 0;

  for (int point = 0; point < this->dem.size; point++) {
    no_of_ring_sectors = size_dsF[point];
    // Every point in the DEM gets a marker saying how much of the proceeding
    // bytes will have ring sector data in them.
    fwrite(&no_of_ring_sectors, 4, 1, fs);
    for (int iRS = 0; iRS < no_of_ring_sectors; iRS++) {
      fwrite(&this->rsectorF[point][iRS], 4, 1, fs);
    }

    no_of_ring_sectors = size_dsB[point];
    fwrite(&no_of_ring_sectors, 4, 1, fs);
    for (int iRS = 0; iRS < no_of_ring_sectors; iRS++) {
      fwrite(&this->rsectorB[point][iRS], 4, 1, fs);
    }
  }

  fclose(fs);

  for (int point = 0; point < this->dem.size; point++) {
    delete[] this->rsectorF[point];
    delete[] this->rsectorB[point];
  }
}

void Sector::progressUpdate() {
  using namespace std::chrono;
  high_resolution_clock::time_point now = high_resolution_clock::now();
  duration<double> elapsed = duration_cast<duration<double>>(now - this->timestamp);
  if (elapsed.count() > 1) {
    printf("sector: %d, point: %d", this->sector_angle, this->bos_manager.current_point);
    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
    fflush(stdout);
    this->timestamp = now;
  }
}

// O(N^2)
void Sector::sweepS() {
  this->sweepInit();
  this->sweepForward();
  this->sweepBackward();
}

void Sector::sweepInit() {
  this->d = this->bos_manager.bos.LL[this->bos_manager.pov].Value.d;
  this->h = this->bos_manager.bos.LL[this->bos_manager.pov].Value.h +
            this->scaled_observer_height;
}

void Sector::sweepForward() {
  int sweep = this->presweepF();
  int sanity = 0;
  if (this->bos_manager.pov != this->bos_manager.bos.Last) {
    while (sweep != -1) {
      this->delta_d = this->bos_manager.bos.LL[sweep].Value.d - d;
      this->delta_h = this->bos_manager.bos.LL[sweep].Value.h - h;
      this->sweeppt = this->bos_manager.bos.LL[sweep].Value.idx;
      this->kernelS(this->rsF, this->nrsF, this->sur_dataF);
      sweep = this->bos_manager.bos.LL[sweep].next;
      sanity++;
      if (sanity > this->dem.size) {
        LOGE << "Forward Bos sweep searching forever";
        throw "Forward BoS sweep didn't find -1";
      }
    }
  }
  this->closeprof(this->visible, true);
}

void Sector::sweepBackward() {
  int sweep = this->presweepB();
  int sanity = 0;
  if (this->bos_manager.pov != this->bos_manager.bos.First) {
    while (sweep != -2) {
      this->delta_d = this->d - this->bos_manager.bos.LL[sweep].Value.d;
      this->delta_h = this->bos_manager.bos.LL[sweep].Value.h - this->h;
      this->sweeppt = this->bos_manager.bos.LL[sweep].Value.idx;
      this->kernelS(this->rsB, this->nrsB, this->sur_dataB);
      sweep = this->bos_manager.bos.LL[sweep].prev;
      sanity++;
      if (sanity > this->dem.size) {
        LOGE << "Backward Bos sweep searching forever";
        throw "Backward BoS sweep didn't find -2";
      }
    }
  }
  this->closeprof(this->visible, false);
}

// remember that rs could be &rsF or &rsB
// O((BW + N)^2) CRITICAL
void Sector::kernelS(int rs[][2], int &nrs, float &sur_data) {
  float angle = this->delta_h / this->delta_d;
  bool above = (angle > this->max_angle);
  bool opening = above && (!this->visible);
  bool closing = (!above) && (this->visible);
  this->visible = above;
  this->max_angle = std::max(angle, this->max_angle);
  if (opening) {
    this->open_delta_d = this->delta_d;
    nrs++;
    rs[nrs][0] = this->sweeppt;
  }
  if (closing) {
    sur_data += (this->delta_d * this->delta_d -
                 this->open_delta_d * this->open_delta_d);
    rs[nrs][1] = this->sweeppt;
  }
}

int Sector::presweepF() {
  this->visible = true;
  this->nrsF = 0;
  this->sur_dataF = 0;
  this->delta_d = 0;
  this->delta_h = 0;
  this->open_delta_d = 0;
  this->open_delta_h = 0;
  int sweep = this->bos_manager.bos.LL[this->bos_manager.pov].next;
  this->sweeppt = this->bos_manager.bos.LL[this->bos_manager.pov].Value.idx;
  this->rsF[0][0] = this->sweeppt;
  this->max_angle = -2000;
  return sweep;
}

int Sector::presweepB() {
  this->visible = true;
  this->nrsB = 0;
  this->sur_dataB = 0;
  this->delta_d = 0;
  this->delta_h = 0;
  this->open_delta_d = 0;
  this->open_delta_h = 0;
  int sweep = this->bos_manager.bos.LL[this->bos_manager.pov].prev;
  this->sweeppt = this->bos_manager.bos.LL[this->bos_manager.pov].Value.idx;
  this->rsB[0][0] = sweeppt;
  this->max_angle = -2000;
  return sweep;
}

// Close any open ring sectors.
//
// I think it's fair to assume that any open ring sectors on the edge of a DEM
// will stretch beyond
// therefore making the TVS calculations for points near the DEM boundaries
// incomplete. You would
// need the DEM stretching as far as any point can possibly see.
//
// NB. The closing of an open profile assumes that boundery DEM points can't be
// seen, which causes
// that point's surface to not be added to the total viewshed surface.
void Sector::closeprof(bool visible, bool fwd) {
  if (fwd) {
    this->nrsF++;
    if (visible) {
      this->rsF[nrsF - 1][1] = this->sweeppt;
      this->sur_dataF += this->delta_d * this->delta_d -
                         this->open_delta_d * this->open_delta_d;
    }
  } else {
    this->nrsB++;
    if (visible) {
      this->rsB[this->nrsB - 1][1] = this->sweeppt;
      this->sur_dataB += this->delta_d * this->delta_d -
                         this->open_delta_d * this->open_delta_d;
    }
  }
}

void Sector::ringSectorDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", RING_SECTOR_DIR.c_str(), sector_angle);
}

void Sector::preComputedDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", SECTOR_DIR.c_str(), sector_angle);
}

}  // namespace TVS
