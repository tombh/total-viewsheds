#include <math.h>

#include <plog/Log.h>

#include "DEM.h"
#include "Axes.h"
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
 * NB: Only square DEMs are currently supported
 *
**/

Axes::Axes(DEM &dem)
    : dem(dem),
      shift_angle(FLAGS_sector_shift),
      isin(new double[dem.size]),
      icos(new double[dem.size]),
      itan(new double[dem.size]),
      icot(new double[dem.size]),
      tmp1(new int[dem.size]),
      tmp2(new int[dem.size]) {}

Axes::~Axes() {
  delete[] this->isin;
  delete[] this->icos;
  delete[] this->itan;
  delete[] this->icot;
  delete[] this->tmp1;
  delete[] this->tmp2;
}

// Translate points to new sector angle
void Axes::adjust(int sector_angle) {
  this->sector_angle = sector_angle;
  this->quad = (this->sector_angle >= 90) ? 1 : 0;
  this->preComputeTrig();
  this->setDistancesFromVerticalAxis();
  this->sortByDistanceFromHorizontalAxis();
}

// Orthogonal distance of points relative to the initial Band of Sight.
// O(N)
void Axes::setDistancesFromVerticalAxis() {
  double val;
  for (int x = 0; x < this->dem.width; x++) {
    for (int y = 0; y < this->dem.height; y++) {
      if (this->quad == 1) {
        val = this->icos[y] - this->isin[x];
      } else {
        val = this->icos[x] + this->isin[y];
      }
      this->dem.nodes[x * this->dem.height + y].d = val;
    }
  }
}

void Axes::sortByDistanceFromHorizontalAxis() {
  this->preSort();
  this->sort();
}

void Axes::preComputeTrig() {
  double sin, cos, tan, cotan;

  this->computable_angle = this->sector_angle;
  if (this->quad == 1) this->computable_angle -= 90;
  this->computable_angle += this->shift_angle;
  this->computable_angle *= TO_RADIANS;

  sin = std::sin(this->computable_angle);
  cos = std::cos(this->computable_angle);
  tan = std::tan(this->computable_angle);
  cotan = 1 / tan;

  // O(N)
  for (int i = 0; i < this->dem.size; i++) {
    this->isin[i] = i * sin;
    this->icos[i] = i * cos;
    this->itan[i] = i * tan;
    this->icot[i] = i * cotan;
  }
}

// TODO: Make readable
void Axes::preSort() {
  double ct, tn;
  this->tmp1[0] = 0;
  this->tmp2[0] = 0;

  tn = std::tan(this->computable_angle);
  ct = 1 / tn;
  // O(N / height)
  for (int j = 1; j < this->dem.width; j++) {
    this->tmp1[j] =
        this->tmp1[j - 1] + (int)std::min(this->dem.height, (int)floor(ct * j));
  }
  // O(N / width)
  for (int i = 1; i < this->dem.height; i++) {
    this->tmp2[i] =
        this->tmp2[i - 1] + (int)std::min(this->dem.width, (int)floor(tn * i));
  }
}

// Sort DEM points in order of their distance from the initial Band of Sight
// line.
// TODO: Make readable
// O(N)
void Axes::sort() {
  double lx = this->dem.width - 1;
  double ly = this->dem.height - 1;
  double x, y;
  int ind, xx, yy, p, np;
  for (int j = 1; j <= this->dem.width; j++) {
    x = (j - 1);
    for (int i = 1; i <= this->dem.height; i++) {
      y = (i - 1);
      ind = i * j;
      ind += ((ly - y) < (this->icot[j - 1]))
                 ? ((this->dem.height - i) * j -
                    this->tmp2[this->dem.height - i] - (this->dem.height - i))
                 : this->tmp1[j - 1];
      ind += ((lx - x) < (itan[i - 1]))
                 ? ((this->dem.width - j) * i -
                    this->tmp1[this->dem.width - j] - (this->dem.width - j))
                 : this->tmp2[i - 1];
      xx = j - 1;
      yy = i - 1;
      p = xx * this->dem.height + yy;
      np = (this->dem.width - 1 - yy) * this->dem.height + xx;
      if (this->quad == 0) {
        this->dem.nodes[p].sight_ordered_index = ind - 1;
        this->dem.nodes_sector_ordered[ind - 1] = np;
      } else {
        this->dem.nodes[np].sight_ordered_index = ind - 1;
        this->dem.nodes_sector_ordered[this->dem.size - ind] = p;
      }
    }
  }
}

}  // namespace TVS
