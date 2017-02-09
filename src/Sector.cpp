#include <math.h>

#include <plog/Log.h>

#include "definitions.h"
#include "DEM.h"
#include "Sector.h"

namespace TVS {

Sector::Sector(DEM &dem)
    : dem(dem),
      // Size of the Band of Sight
      band_size(FLAGS_dem_width),
      half_band_size((band_size - 1) / 2),
      // TODO: Explain all this somwhere
      shift_angle(FLAGS_sector_shift),
      // Scaling all measurements to a single unit of the DEM_SCALE makes
      // calculations simpler.
      // TODO: I think this could be a problem when considering DEM's without
      // curved earth projections?
      scaled_observer_height(FLAGS_observer_height / dem.scale)
  {}

void Sector::init_storage() {
  this->nodes = new LinkedList::node[this->dem.size];
  this->band_of_sight = LinkedList(this->band_size);

  this->isin = new double[this->dem.size];
  this->icos = new double[this->dem.size];
  this->icot = new double[this->dem.size];
  this->itan = new double[this->dem.size];

  this->orderiAT = new int[this->dem.size];
  this->tmp1 = new int[this->dem.size];
  this->tmp2 = new int[this->dem.size];
  if (this->dem.is_computing) {
    this->surfaceF = new float[this->dem.size];
    this->surfaceB = new float[this->dem.size];
    // TODO: sometimes surfaceB doesn't get any surface for
    // some points. Is this reasonable or a bug, perhaps in
    // absent nodes from the line of sight?
    // So for now, just initialise all items to 0
    for(int point = 0; point < this->dem.size; point++){
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
  delete[] this->nodes;

  // TODO: it should be LinkedList's responsibility to delete this.
  // But using LinkedList's destructor causes LL to be deleted prematurely.
  delete[] this->band_of_sight.LL;

  delete[] this->isin;
  delete[] this->icos;
  delete[] this->icot;
  delete[] this->itan;

  delete[] this->orderiAT;
  delete[] this->tmp1;
  delete[] this->tmp2;

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

void Sector::setHeights() {
  FILE *f;
  unsigned short num;
  f = fopen(INPUT_DEM_FILE.c_str(), "rb");
  if (f == NULL) {
    LOG_ERROR << "Error opening: " << INPUT_DEM_FILE;
  } else {
    for (int point = 0; point < this->dem.size; point++) {
      this->nodes[point].idx = point;

      fread(&num, 2, 1, f);

      // Original reading does this funny geometric translation...
      // It's like the DEM is a piece of paper and you flip the top-right corner
      // to the bottom-left. But why!?
      // h[i/xdim+ydim*(i%xdim)] = num/SCALE; //internal representation from top
      // to row.

      // Why divide by SCALE? Is it multiplied in again later for the total
      // viewshed surface area calculation?
      this->nodes[point].h = (float)num / this->dem.scale;  // decameters
    }
  }
}

void Sector::loopThroughBands(int sector_angle) {
  bool starting, ending;
  this->sector_angle = sector_angle;
  this->openPreComputedDataFile();
  this->changeAngle();
  for (int point = 0; point < this->dem.size; point++) {
    this->PoV = point % this->band_size;
    if (this->dem.is_computing) {
      this->sweepS(point);
      this->storers(this->orderiAT[point]);
    }
    this->post_loop(point);
  }
  fclose(this->precomputed_data_file);
}

void Sector::changeAngle() {
  LOGD << "Changing sector angle to: " << this->sector_angle;
  this->quad = (this->sector_angle >= 90) ? 1 : 0;
  if (this->quad == 1) this->sector_angle -= 90;
  this->pre_computed_sin();
  this->sort();
  if (this->quad == 1) this->sector_angle += 90;
  this->setDistances(this->sector_angle);
  this->band_of_sight.Clear();
  this->band_of_sight.FirstNode(this->nodes[this->orderiAT[0]]);
}

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

void Sector::pre_computed_sin() {
  double ac;
  double s, c, ct, tn;

  ac = (this->shift_angle + this->sector_angle + 0.5) * TO_RADIANS;
  this->sect_angle = ac;
  s = std::sin(this->sect_angle);
  c = std::cos(this->sect_angle);
  tn = std::tan(this->sect_angle);
  ct = 1 / tn;

  // O(N)
  for (int i = 0; i < this->dem.size; i++) {
    this->isin[i] = i * s;
    this->icos[i] = i * c;
    this->icot[i] = i * ct;
    this->itan[i] = i * tn;
  }
}

// Orthogonal distance of point relative to the initial point of the Band of Sight.
// O(N)
void Sector::setDistances(int sector) {
  double val;
  for (int j = 0; j < this->dem.width; j++) {
    for (int i = 0; i < this->dem.height; i++) {
      val = icos[j] + isin[i];
      if (quad == 1) val = icos[i] - isin[j];
      nodes[j * this->dem.height + i].d = val;
    }
  }
}

void Sector::presort() {
  double ct, tn;
  this->tmp1[0] = 0;
  this->tmp2[0] = 0;

  tn = std::tan(this->sect_angle);
  ct = 1 / tn;
  // O(N)
  for (int j = 1; j < this->dem.width; j++) {
    this->tmp1[j] =
        this->tmp1[j - 1] + (int)std::min(this->dem.height, (int)floor(ct * j));
  }
  // O(N)
  for (int i = 1; i < this->dem.height; i++) {
    this->tmp2[i] =
        this->tmp2[i - 1] + (int)std::min(this->dem.width, (int)floor(tn * i));
  }
}

// Sort DEM points in order of their distance from the initial Band of Sight line.
// O(N^2)
void Sector::sort() {
  this->presort();
  double lx = this->dem.width - 1;
  double ly = this->dem.height - 1;
  double x, y;
  int ind, xx, yy, p, np;
  for (int j = 1; j <= this->dem.width; j++) {
    x = (j - 1);
    for (int i = 1; i <= this->dem.height; i++) {
      y = (i - 1);
      ind = i * j;
      ind += ((ly - y) < (icot[j - 1]))
                 ? ((this->dem.height - i) * j - this->tmp2[this->dem.height - i] -
                    (this->dem.height - i))
                 : this->tmp1[j - 1];
      ind += ((lx - x) < (itan[i - 1]))
                 ? ((this->dem.width - j) * i - this->tmp1[this->dem.width - j] -
                    (this->dem.width - j))
                 : this->tmp2[i - 1];
      xx = j - 1;
      yy = i - 1;
      p = xx * this->dem.height + yy;
      np = (this->dem.width - 1 - yy) * this->dem.height + xx;
      if (this->quad == 0) {
        this->nodes[p].oa = ind - 1;
        this->orderiAT[ind - 1] = np;
      } else {
        this->nodes[np].oa = ind - 1;
        this->orderiAT[this->dem.size - ind] = p;
      }
    }
  }
}

void Sector::post_loop(int point) {
  // Is the band of sight starting to be populated?
  bool starting = (point < this->half_band_size);
  // Is the band of sight coming to an ending, therefore emptying?
  bool ending = (point >= (this->dem.size - this->half_band_size - 1));

  this->atright = false;
  this->atright_trans = false;
  this->NEW_position = -1;
  this->NEW_position_trans = -1;
  LinkedList::node tmp = this->newnode_trans;
  if (!ending) {
    this->newnode =
        this->nodes[this->orderiAT[starting ? 2 * point + 1
                                            : this->half_band_size + point + 1]];
    this->atright =
        this->newnode.oa > this->band_of_sight.LL[this->PoV].Value.oa;
  }
  if (starting) {
    this->newnode_trans = this->nodes[this->orderiAT[2 * point + 2]];
    this->atright_trans =
        this->newnode_trans.oa > this->band_of_sight.LL[this->PoV].Value.oa;
  }

  if (!this->dem.is_precomputing) {
    if (!ending) {
      fread(&this->NEW_position, 4, 1, this->precomputed_data_file);
      this->insert_node(this->newnode, this->NEW_position, !starting);
    }
    if (starting) {
      fread(&this->NEW_position_trans, 4, 1, this->precomputed_data_file);
      this->insert_node(this->newnode_trans, this->NEW_position_trans, false);
    }
  } else {
    if (!starting && !ending) {
      this->newnode =
          this->nodes[this->orderiAT[starting ? 2 * point + 1
                                              : this->half_band_size + point + 1]];
      this->NEW_position = -1;
      this->atright =
          this->newnode.oa > this->band_of_sight.LL[this->PoV].Value.oa;
      this->calcule_pos_newnode(true);
    }
    if (starting) {
      this->newnode = this->nodes[this->orderiAT[2 * point + 1]];
      this->NEW_position = -1;
      this->atright = this->newnode.oa > this->band_of_sight.LL[PoV].Value.oa;
      this->calcule_pos_newnode(false);
      this->newnode = this->nodes[this->orderiAT[2 * point + 2]];
      this->atright =
          this->newnode.oa > this->band_of_sight.LL[this->PoV].Value.oa;
      this->calcule_pos_newnode(false);
    }
  }
  if (ending) this->band_of_sight.Remove_two();
}

void Sector::calcule_pos_newnode(bool remove) {
  int sweep = 0;
  if (newnode.oa < this->band_of_sight.LL[this->band_of_sight.First].Value.oa) {
    NEW_position = -2;  // Before first
    this->band_of_sight.AddFirst(newnode, remove);
    fwrite(&NEW_position, 4, 1, this->precomputed_data_file);
  } else if (newnode.oa >
             this->band_of_sight.LL[this->band_of_sight.Last].Value.oa) {
    NEW_position = -1;
    this->band_of_sight.AddLast(newnode, remove);
    fwrite(&NEW_position, 4, 1, this->precomputed_data_file);
  } else {
    sweep = this->band_of_sight.LL[this->band_of_sight.First].next;
    bool go_on = true;
    int sanity = 0;

    do {
      if (newnode.oa < this->band_of_sight.LL[sweep].Value.oa) {
        NEW_position = this->band_of_sight.LL[sweep].prev;
        this->band_of_sight.Add(newnode, NEW_position, remove);
        fwrite(&NEW_position, 4, 1, this->precomputed_data_file);
        sweep = this->band_of_sight.LL[NEW_position].next;
        go_on = false;
      } else {
        sweep = this->band_of_sight.LL[sweep].next;
        sanity++;
        if(sanity > this->dem.size) {
          LOGE << "newnode.oa: " << newnode.oa << " is bigger than anything in band of sight.";
          //throw "Couldn't find position for new node";
          go_on = false;
        }
      }
    } while (go_on);
  }
}

void Sector::insert_node(LinkedList::node newnode, int position, bool remove) {
  if (position > -1) {
    this->band_of_sight.Add(newnode, position, remove);
  } else {
    if (position == -1) {
      this->band_of_sight.AddLast(newnode, remove);
    } else {
      this->band_of_sight.AddFirst(newnode, remove);
    }
  }
}

// Fulldata to store ring sector topology
void Sector::storers(int i) {
  float surscale = PI_F / (360 * this->dem.scale * this->dem.scale);  // hectometers^2
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

  this->rsectorF[orderiAT[this->dem.size - 1]] = new int[0];
  this->rsectorB[orderiAT[this->dem.size - 1]] = new int[0];
  this->size_dsB[orderiAT[this->dem.size - 1]] = 0;
  this->size_dsF[orderiAT[this->dem.size - 1]] = 0;

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

// O(N^2)
void Sector::sweepS(int _not_used) {
  int sweep;
  int sanity;

  float d = this->band_of_sight.LL[PoV].Value.d;
  float h = this->band_of_sight.LL[PoV].Value.h + this->scaled_observer_height;

  sweep = this->presweepF();
  sanity = 0;
  if (this->PoV != this->band_of_sight.Last) {
    while (sweep != -1) {
      this->delta_d = this->band_of_sight.LL[sweep].Value.d - d;
      this->delta_h = this->band_of_sight.LL[sweep].Value.h - h;
      this->sweeppt = this->band_of_sight.LL[sweep].Value.idx;
      this->kernelS(this->rsF, this->nrsF, this->sur_dataF);
      sweep = this->band_of_sight.LL[sweep].next;
      sanity++;
      if(sanity > this->dem.size) {
        LOGE << "Forward Bos sweep searching forever";
        throw "Forward BoS sweep didn't find -1";
      }
    }
  }
  this->closeprof(this->visible, true, false, false);

  sweep = this->presweepB();
  sanity = 0;
  if (this->PoV != this->band_of_sight.First) {
    while (sweep != -2) {
      this->delta_d = d - this->band_of_sight.LL[sweep].Value.d;
      this->delta_h = this->band_of_sight.LL[sweep].Value.h - h;
      this->sweeppt = this->band_of_sight.LL[sweep].Value.idx;
      this->kernelS(this->rsB, this->nrsB, this->sur_dataB);
      sweep = this->band_of_sight.LL[sweep].prev;
      sanity++;
      if(sanity > this->dem.size) {
        LOGE << "Backward Bos sweep searching forever";
        throw "Backward BoS sweep didn't find -2";
      }
    }
  }
  this->closeprof(this->visible, false, false, false);
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
    sur_data += (this->delta_d * this->delta_d - this->open_delta_d * this->open_delta_d);
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
  int sweep = this->band_of_sight.LL[PoV].next;
  this->sweeppt = this->band_of_sight.LL[PoV].Value.idx;
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
  int sweep = this->band_of_sight.LL[PoV].prev;
  this->sweeppt = this->band_of_sight.LL[PoV].Value.idx;
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
void Sector::closeprof(bool visible, bool fwd, bool vol, bool full) {
  if (fwd) {
    this->nrsF++;
    if (visible) {
      this->rsF[nrsF - 1][1] = this->sweeppt;
      this->sur_dataF += this->delta_d * this->delta_d - this->open_delta_d * this->open_delta_d;
    }

    // I think this means, "discount bands of sight that contain only one ring
    // sector and where the PoV is close to the edge". But I don't know why and
    // also I don't know why the ring sector is removed but the surface area is
    // stil counted.
    if ((this->nrsF == 1) && (this->delta_d < 1.5F)) this->nrsF = 0;
  } else {
    this->nrsB++;
    if (visible) {
      this->rsB[this->nrsB - 1][1] = this->sweeppt;
      this->sur_dataB += this->delta_d * this->delta_d - this->open_delta_d * this->open_delta_d;
    }
    if ((this->nrsB == 1) && (this->delta_d < 1.5F)) this->nrsB = 0;  // safe data with small vs
  }
}

void Sector::ringSectorDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", RING_SECTOR_DIR.c_str(), sector_angle);
}

void Sector::preComputedDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", SECTOR_DIR.c_str(), sector_angle);
}

}  // namespace TVS
