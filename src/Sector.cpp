#include <math.h>
#include <chrono>

#include <plog/Log.h>

#include "Axes.h"
#include "BOS.h"
#include "DEM.h"
#include "Sector.h"
#include "Viewsheds.h"
#include "definitions.h"

namespace TVS {

Sector::Sector(DEM &dem) : dem(dem), bos_manager(BOS(dem)), viewsheds(Viewsheds(dem, bos_manager)) {}

void Sector::prepareForPreCompute() {
  this->bos_manager.initBandStorage();
}

void Sector::prepareForCompute() {
  this->bos_manager.initBandStorage();
  this->viewsheds.computable_band_size = this->bos_manager.computable_band_size;
  this->viewsheds.computable_bands = this->dem.computable_points_count * 2;
  this->viewsheds.initialise();
}

void Sector::changeAngle(int sector_angle) {
  this->sector_angle = sector_angle;
  Axes axes = Axes(this->dem);
  axes.adjust(sector_angle);
}

void Sector::sweepThroughAllBands() {
  this->bos_manager.setup(this->sector_angle);
  for (int i = 0; i < this->dem.size; i++) {
    this->bos_manager.adjustToNextPoint(i);
    if (this->dem.isPointComputable(this->dem.sector_ordered[i])) {
      this->bos_manager.buildComputableBands();
    }
  }
  this->bos_manager.writeAndClose();
}

void Sector::calculateViewsheds() {
  this->viewsheds.sector_angle = this->sector_angle;
  this->viewsheds.transferSectorData();
  this->viewsheds.calculate();
}

}  // namespace TVS
