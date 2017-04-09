#include <thread>

#include <plog/Log.h>

#include "Clamp.h"
#include "DEM.h"
#include "Viewsheds.h"
#include "definitions.h"
#include "helper.h"

namespace TVS {

Viewsheds::Viewsheds(DEM &dem, BOS &bos)
    : dem(dem),
      bos(bos),
      reserved_rings(FLAGS_reserved_ring_space * 2),
      computed_sectors(0),
      batches(FLAGS_gpu_batches),
      ring_writer_threads(new std::thread[FLAGS_total_sectors * FLAGS_gpu_batches]),
      cl(Clamp()),
      gpu(cl.createDev(FLAGS_cl_device)) {}

void Viewsheds::initialise() {
  LOGI << "Using max line of sight of: " << FLAGS_max_line_of_sight;
  this->removeRingData();
  this->prepareKernel();
  this->prepareMemory();
}

void Viewsheds::ringDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", RING_SECTOR_DIR.c_str(), sector_angle);
}

// Remove old ring data because files get appended later on.
void Viewsheds::removeRingData() {
  std::stringstream cmd;
  if (std::string(RING_SECTOR_DIR).find(".") != 0) {
    throw "Not deleting unexpected RING_SECTOR_DIR path";
  }
  cmd << "rm -rf " << RING_SECTOR_DIR;
  system(cmd.str().c_str());
  helper::createDirectory(RING_SECTOR_DIR);
}

void Viewsheds::prepareKernel() {
  std::stringstream extra;
  extra << " -D BAND_DELTAS_SIZE=" << this->bos.band_deltas_size;
  extra << " -D TOTAL_BANDS=" << this->bos.band_count;
  extra << " -D TVS_WIDTH=" << this->dem.tvs_width;
  extra << " -D DEM_WIDTH=" << this->dem.width;
  extra << " -D MAX_LOS_AS_POINTS=" << this->dem.max_los_as_points;
  extra << " -D OBSERVER_HEIGHT=" << FLAGS_observer_height;
  extra << " -D RESERVED_RINGS=" << this->reserved_rings;
  this->program = this->cl.compileProgram("src/viewshed.cl", extra.str().c_str());
  this->kernel = this->program->getKernel("viewshed");
}

void Viewsheds::prepareMemory() {
  this->cumulative_surfaces = this->gpu->allocMem(sizeof(float), this->bos.band_count);
  this->gpu->fillWithZeroes(this->cumulative_surfaces);

  this->ring_data_size = (this->bos.band_count / this->batches) * this->reserved_rings;
  this->sector_rings = this->gpu->allocMem(sizeof(int), this->ring_data_size);

  this->distances = this->gpu->allocMem(sizeof(float), this->dem.size);
  this->elevations = this->gpu->allocMem(sizeof(short), this->dem.size);
  this->band_deltas = this->gpu->allocMem(sizeof(int), this->bos.band_deltas_size);

  // Elevations can be sent now because they never change.
  this->gpu->write(this->elevations, this->dem.elevations);
}

void Viewsheds::calculate(int angle) {
  this->sector_angle = angle;
  int applied_domain_size;
  int batch_start = 0;
  int domain_size = this->bos.band_count / this->batches;
  int remainder = this->bos.band_count % this->batches;


  this->transferSectorData();
  for (int batch = 0; batch < this->batches; batch++) {
    // Spread the remainder amongst existing batches, rather
    // than having a tiny batch at the end.
    if (batch < remainder) {
      applied_domain_size = domain_size + 1;
    } else {
      applied_domain_size = domain_size;
    }
    this->kernel->setDomain(applied_domain_size);
    this->kernel->setArg(5, batch_start);
    batch_start += applied_domain_size;
    this->gpu->queue(this->kernel);
    this->gpu->wait();
    this->writeRingData();
  }
}

void Viewsheds::transferSectorData() {
  this->gpu->write(this->distances, this->dem.distances);
  this->bos.loadBandData(this->sector_angle);
  this->gpu->write(this->band_deltas, this->bos.band_deltas);
  this->kernel->setArg(0, this->elevations);
  this->kernel->setArg(1, this->distances);
  this->kernel->setArg(2, this->band_deltas);
  this->kernel->setArg(3, this->cumulative_surfaces);
  this->kernel->setArg(4, this->sector_rings);
}

void Viewsheds::transferToHost() {
  float *surfaces = new float[this->bos.band_count];

  this->gpu->read(this->cumulative_surfaces, surfaces);
  for (int i = 0; i < this->dem.computable_points_count; i++) {
    // Front bands
    this->dem.tvs_complete[i] = surfaces[i];
    // Back bands
    this->dem.tvs_complete[i] += surfaces[i + this->dem.computable_points_count];
  }

  for (int i = 0; i < this->computed_sectors; i++) {
    this->ring_writer_threads[i].join();
  }

  delete[] surfaces;
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
  FILE *file = fopen(path, "ab"); // TODO: *KLAXON* OMG appending is NOT thread safe!
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
