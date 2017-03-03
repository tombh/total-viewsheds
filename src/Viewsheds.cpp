#include <plog/Log.h>

#include "Clamp.h"
#include "DEM.h"
#include "Viewsheds.h"
#include "definitions.h"

namespace TVS {

Viewsheds::Viewsheds(DEM &dem)
  : dem(dem),
    cl(Clamp()),
    gpu(cl.createDev(FLAGS_cl_device)),
    program(cl.compileProgram("src/viewshed.cl")),
    kernel(program->getKernel("viewshed")) {}

void Viewsheds::ringSectorDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", RING_SECTOR_DIR.c_str(), sector_angle);
}

void Viewsheds::initialise() {
  this->cumulative_surfaces = this->gpu->allocMem(sizeof(float), this->computable_bands);
  this->distances = this->gpu->allocMem(sizeof(float), this->dem.size);
  this->elevations = this->gpu->allocMem(sizeof(float), this->dem.size);
  this->bands = this->gpu->allocMem(sizeof(int), this->total_band_points);
  this->gpu->write(this->elevations, this->dem.elevations);
}

void Viewsheds::calculate(int sector_angle) {
  this->sector_angle = sector_angle;
  this->kernel->setDomain(this->computable_bands);
  this->kernel->setArg(0, this->elevations);
  this->kernel->setArg(1, this->distances);
  this->kernel->setArg(2, this->bands);
  this->kernel->setArg(3, this->computable_band_size);
  this->kernel->setArg(4, FLAGS_max_line_of_sight / this->dem.scale);
  this->kernel->setArg(5, this->dem.width);
  this->kernel->setArg(6, this->dem.tvs_width);
  this->kernel->setArg(7, this->dem.computable_points_count);
  this->kernel->setArg(8, this->cumulative_surfaces);
  this->gpu->queue(this->kernel);
  this->gpu->wait();
}

void Viewsheds::transferSectorData(BOS &bos) {
  this->gpu->write(this->distances, this->dem.distances);
  this->gpu->write(this->bands, bos.bands);
}

void Viewsheds::transferToHost() {
  float *fb_surfaces = new float[this->computable_bands];
  this->gpu->read(this->cumulative_surfaces, fb_surfaces);
  for(int i=0; i < this->dem.computable_points_count; i++) {
    this->dem.tvs_complete[i] = fb_surfaces[i];
    this->dem.tvs_complete[i] += fb_surfaces[i + this->dem.computable_points_count];
  }
  delete[] fb_surfaces;
}

}  // namespace TVS
