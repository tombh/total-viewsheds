#include <math.h>

#include <plog/Log.h>

#include "DEM.h"
#include "SectorAxes.h"
#include "definitions.h"

namespace TVS {

/**
 * Calculate DEM properties based on a new set of axes.
 *
 * It's easy to imagine the following DEM points based around an
 * intuitive x/y horizontally/vertically aligned axis. But let's
 * overlay another axis (ab/cd) rotated at an angle, namely the
 * so-called "sector angle". Here the sector angle is 45 degreees:
 *
 *       y
 *       ^
 *       |
 *     a |                         d
 *       *  .  .  .  .  .  .  .  *
 *       .  *  .  .  .  .  .  *  .
 *       .  .  *  .  .  .  *  .  .
 *       .  .  .  *  .  *  .  .  .
 *       .  .  .  .  *  .  .  .  .
 *       .  .  .  *  .  *  .  .  .
 *       .  .  *  .  .  .  *  .  .
 *       .  *  .  .  .  .  .  *  .
 *       *  .  .  .  .  .  .  .  * -----> x
 *     c                           b
 *
 * Roughly speaking, the Band of Sight always runs parallel to the 'ab'
 * axis and we use the 'cd' axis as a datum to measure relative
 * distances inside the Band of Sight. In reality the 2 new axes do not
 * necessarily intersect in the middle of the DEM.
 *
**/

SectorAxes::SectorAxes(Sector &sector)
    : sector(sector),
      isin(new double[sector.dem.size]),
      icos(new double[sector.dem.size]),
      itan(new double[sector.dem.size]),
      icot(new double[sector.dem.size]),
      tmp1(new int[sector.dem.size]),
      tmp2(new int[sector.dem.size]) {}

SectorAxes::~SectorAxes() {
  delete[] this->isin;
  delete[] this->icos;
  delete[] this->itan;
  delete[] this->icot;
  delete[] this->tmp1;
  delete[] this->tmp2;
}

// Translate points to new sector angle
void SectorAxes::adjust() {
  this->preComputeTrig();
  this->setDistancesFromVerticalAxis();
  this->sortByDistanceFromHorizontalAxis();
}

// Orthogonal distance of points relative to the initial Band of Sight.
// O(N)
void SectorAxes::setDistancesFromVerticalAxis() {
  double val;
  for (int x = 0; x < this->sector.dem.width; x++) {
    for (int y = 0; y < this->sector.dem.height; y++) {
      if (this->sector.quad == 1) {
        val = this->icos[y] - this->isin[x];
      } else {
        val = this->icos[x] + this->isin[y];
      }
      this->sector.nodes[x * this->sector.dem.height + y].d = val;
    }
  }
}

void SectorAxes::sortByDistanceFromHorizontalAxis() {
  this->preSort();
  this->sort();
}

void SectorAxes::preComputeTrig() {
  double sin, cos, tan, cotan;

  this->computable_angle = this->sector.sector_angle;
  if (this->sector.quad == 1) this->computable_angle -= 90;
  this->computable_angle += this->sector.shift_angle;
  this->computable_angle += 0.5; // TODO: What is this?
  this->computable_angle *= TO_RADIANS;

  sin = std::sin(this->computable_angle);
  cos = std::cos(this->computable_angle);
  tan = std::tan(this->computable_angle);
  cotan = 1 / tan;

  // O(N)
  for (int i = 0; i < this->sector.dem.size; i++) {
    this->isin[i] = i * sin;
    this->icos[i] = i * cos;
    this->itan[i] = i * tan;
    this->icot[i] = i * cotan;
  }
}

// TODO: Make readable
void SectorAxes::preSort() {
  double ct, tn;
  this->tmp1[0] = 0;
  this->tmp2[0] = 0;

  tn = std::tan(this->computable_angle);
  ct = 1 / tn;
  // O(N / height)
  for (int j = 1; j < this->sector.dem.width; j++) {
    this->tmp1[j] =
        this->tmp1[j - 1] + (int)std::min(this->sector.dem.height, (int)floor(ct * j));
  }
  // O(N / width)
  for (int i = 1; i < this->sector.dem.height; i++) {
    this->tmp2[i] =
        this->tmp2[i - 1] + (int)std::min(this->sector.dem.width, (int)floor(tn * i));
  }
}

// Sort DEM points in order of their distance from the initial Band of Sight
// line.
// TODO: Make readable
// O(N)
void SectorAxes::sort() {
  double lx = this->sector.dem.width - 1;
  double ly = this->sector.dem.height - 1;
  double x, y;
  int ind, xx, yy, p, np;
  for (int j = 1; j <= this->sector.dem.width; j++) {
    x = (j - 1);
    for (int i = 1; i <= this->sector.dem.height; i++) {
      y = (i - 1);
      ind = i * j;
      ind += ((ly - y) < (this->icot[j - 1]))
                 ? ((this->sector.dem.height - i) * j -
                    this->tmp2[this->sector.dem.height - i] - (this->sector.dem.height - i))
                 : this->tmp1[j - 1];
      ind += ((lx - x) < (itan[i - 1]))
                 ? ((this->sector.dem.width - j) * i -
                    this->tmp1[this->sector.dem.width - j] - (this->sector.dem.width - j))
                 : this->tmp2[i - 1];
      xx = j - 1;
      yy = i - 1;
      p = xx * this->sector.dem.height + yy;
      np = (this->sector.dem.width - 1 - yy) * this->sector.dem.height + xx;
      if (this->sector.quad == 0) {
        this->sector.nodes[p].oa = ind - 1;
        this->sector.nodes_orth_ordered[ind - 1] = np;
      } else {
        this->sector.nodes[np].oa = ind - 1;
        this->sector.nodes_orth_ordered[this->sector.dem.size - ind] = p;
      }
    }
  }
}

}  // namespace TVS
