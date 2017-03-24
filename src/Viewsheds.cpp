#include <thread>

#include <plog/Log.h>

#include "Clamp.h"
#include "DEM.h"
#include "Viewsheds.h"
#include "definitions.h"

namespace TVS {

Viewsheds::Viewsheds(DEM &dem, BOS &bos_manager)
    : dem(dem),
      bos_manager(bos_manager),
      reserved_rings(FLAGS_reserved_ring_space * 2),
      computed_sectors(0),
      ring_writer_threads(new std::thread[FLAGS_total_sectors]),
      cl(Clamp()),
      gpu(cl.createDev(FLAGS_cl_device)) {}

void Viewsheds::ringDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", RING_SECTOR_DIR.c_str(), sector_angle);
}

void Viewsheds::initialise() {
  std::stringstream extra;
  LOGI << "Using max line of sight of: " << FLAGS_max_line_of_sight;
  this->total_band_points = this->computable_bands * this->computable_band_size;
  // Passing the band size as a compiler argument allows for optimisation through
  // loop unrolling.
  extra << " -D BAND_SIZE=" << this->computable_band_size;
  extra << " -D TOTAL_BANDS=" << this->computable_bands;
  extra << " -D TVS_WIDTH=" << this->dem.tvs_width;
  extra << " -D DEM_WIDTH=" << this->dem.width;
  extra << " -D MAX_LOS_AS_POINTS=" << this->dem.max_los_as_points;
  extra << " -D OBSERVER_HEIGHT=" << FLAGS_observer_height;
  extra << " -D RESERVED_RINGS=" << this->reserved_rings;
  this->program = this->cl.compileProgram("src/viewshed.cl", extra.str().c_str());
  this->kernel = this->program->getKernel("viewshed");
  this->ring_data_size = this->computable_bands * this->reserved_rings;
  this->cumulative_surfaces = this->gpu->allocMem(sizeof(float), this->computable_bands);
  this->gpu->fillWithZeroes(this->cumulative_surfaces);
  this->sector_rings = this->gpu->allocMem(sizeof(int), this->ring_data_size);
  this->distances = this->gpu->allocMem(sizeof(float), this->dem.size);
  this->elevations = this->gpu->allocMem(sizeof(float), this->dem.size);
  this->band_markers = this->gpu->allocMem(sizeof(int), this->computable_bands);
  this->gpu->write(this->elevations, this->dem.elevations);
}

void Viewsheds::calculate() {
  this->kernel->setDomain(this->computable_bands);
  this->kernel->setArg(0, this->elevations);
  this->kernel->setArg(1, this->distances);
  this->kernel->setArg(2, this->bands);
  this->kernel->setArg(3, this->band_markers);
  this->kernel->setArg(4, this->cumulative_surfaces);
  this->kernel->setArg(5, this->sector_rings);
  this->gpu->queue(this->kernel);
  this->gpu->wait();
  this->writeRingData();
  // Free the GPU band memory because its size is different for every sector
  this->gpu->freeMem(this->bands);
}

void Viewsheds::transferSectorData() {
  // Distance data
  this->gpu->write(this->distances, this->dem.distances);
  // Band data
  this->bos_manager.adjustToAngle(this->sector_angle);
  this->bos_manager.loadBandData();
  this->gpu->write(this->band_markers, this->bos_manager.band_markers);
  this->bands = this->gpu->allocMem(sizeof(short), this->bos_manager.band_data_size);
  this->gpu->write(this->bands, this->bos_manager.band_data);
}

void Viewsheds::transferToHost() {
  float *fb_surfaces = new float[this->computable_bands];

  this->gpu->read(this->cumulative_surfaces, fb_surfaces);
  for (int i = 0; i < this->dem.computable_points_count; i++) {
    this->dem.tvs_complete[i] = fb_surfaces[i];
    this->dem.tvs_complete[i] += fb_surfaces[i + this->dem.computable_points_count];
  }

  for (int i = 0; i < this->computed_sectors; i++) {
    this->ring_writer_threads[i].join();
  }

  delete[] fb_surfaces;
  delete[] this->ring_writer_threads;
}

// My assumption for making this a static function (not requiring `this`) is that thread()
// makes a *copy* of `this`, which in our case duplicates a lot of data. So a static function
// saves memory.
void Viewsheds::threadedWrite(FILE *file, int *ring_data, int data_size, int step) {
  int band_ring_size;
  for (int band = 0; band < data_size; band += step) {
    // More space than is needed is reserved for each band's ring data,
    // so don't write the unused data.
    band_ring_size = ring_data[band] + 1;
    int data[band_ring_size];
    for (int i = 0; i < band_ring_size; i++) {
      data[i] = ring_data[band + i];
    }
    fwrite(data, 4, band_ring_size, file);
  }
  fclose(file);
  delete[] ring_data;
}

void Viewsheds::writeRingData() {
  char path[100];
  int *ring_data = new int[this->ring_data_size];

  LOGI << "Writing Ring Sector data ...";

  this->ringDataPath(path, this->sector_angle);
  FILE *file = fopen(path, "wb");
  if (file == NULL) {
    LOG_ERROR << "Couldn't open " << path << " for sector_angle: " << this->sector_angle;
    throw std::runtime_error("Couldn't open file.");
  }

  this->gpu->read(this->sector_rings, ring_data);
  this->ring_writer_threads[this->computed_sectors] = std::thread(
    Viewsheds::threadedWrite, file, ring_data, this->ring_data_size, this->reserved_rings
  );
  this->computed_sectors++;
}

}  // namespace TVS
