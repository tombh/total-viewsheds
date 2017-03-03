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

Sector::Sector(DEM &dem) : dem(dem), bos_manager(BOS(dem)), viewsheds(Viewsheds(dem)) {}

void Sector::prepareForCompute() {
  this->viewsheds.computable_band_size = this->bos_manager.computable_band_size;
  this->viewsheds.computable_bands = this->dem.computable_points_count * 2;
  this->viewsheds.total_band_points = viewsheds.computable_band_size * viewsheds.computable_bands;
  this->viewsheds.transferDEMData();
}

void Sector::changeAngle(int sector_angle) {
  LOGI << "Changing sector angle to: " << sector_angle;
  this->sector_angle = sector_angle;
  Axes axes = Axes(this->dem);
  axes.adjust(sector_angle);
}

void Sector::loopThroughBands() {
  this->bos_manager.setup(this->sector_angle);
  for (int i = 0; i < this->dem.size; i++) {
    this->bos_manager.adjustToNextPoint(i);
    if (this->dem.is_computing) {
      this->bos_manager.buildBands(this->dem.sector_ordered[i]);
    }
  }
  this->bos_manager.writeAndClose();
}

void Sector::calculateViewsheds() {
  this->viewsheds.transferSectorData(bos_manager);
  this->viewsheds.calculate(this->sector_angle);
}

}  // namespace TVS
