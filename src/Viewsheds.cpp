#include <arrayfire.h>

#include <plog/Log.h>

#include "DEM.h"
#include "Viewsheds.h"
#include "definitions.h"

namespace TVS {

Viewsheds::Viewsheds(DEM &dem) : dem(dem) {}

void Viewsheds::ringSectorDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", RING_SECTOR_DIR.c_str(), sector_angle);
}

void Viewsheds::timerStart() {
  af::sync();
  this->mark = af::timer::start();
}

void Viewsheds::timerLog(const char tag) {
  char *time = NULL;
  af::sync();
  asprintf(&time, "%s: %g", tag, af::timer::stop(this->mark));
  LOGI << time;
  free(time);
}


void Viewsheds::calculate(int sector_angle) {
  this->sector_angle = sector_angle;
  af::setDevice(FLAGS_af_device);
  this->hullPass();
  this->findVisiblePoints();
  this->sumSurfaces();
  this->findRingSectors();
}

// Transfering data between RAM and GPU. Rule of thumb is that this happens at about 5GB/s.
void Viewsheds::transferDEMData() {
  this->cumulative_surfaces = af::constant(0.0, this->dem.computable_points_count, u32);
  this->elevations = af::array(this->dem.size, this->dem.elevations);
}

void Viewsheds::transferSectorData(BOS &bos) {
  this->distances = af::array(this->dem.size, this->dem.distances);
  this->bands = af::array(this->total_band_points, bos.bands);
}

void Viewsheds::hullPass() {
  // Populate an array with all the elevations
  af::array bos_elevations = this->elevations(this->bands);

  bos_elevations = moddims(bos_elevations, this->computable_band_size, this->computable_bands);

  // Pick off just the first point of each band for the PoV
  af::array pov_elevations = bos_elevations(0, af::span);

  // Calculate the difference in height between the PoV and each point in the band
  af::array elevation_deltas = bos_elevations - af::tile(pov_elevations, this->computable_band_size);

  // Populate an array with all the distances
  af::array bos_distances = this->distances(this->bands);
  bos_distances = moddims(bos_distances, this->computable_band_size, this->computable_bands);

  // Pick off just the first point of each band for the PoV
  af::array pov_distances = bos_distances(0, af::span);

  // Calculate the distance between the PoV and each point in the band
  this->distance_deltas = af::abs(bos_distances - af::tile(pov_distances, this->computable_band_size));

  // Calculate the angle of elevation for each point in the band.
  // Note the adjustment for curvature of the earth. It is merely a crude
  // approximation using the spherical earth model.
  // TODO: The PoVs incur 0/0 division, giving minus VERYVERYBIG
  // TODO: Account for refraction.
  this->angles = (elevation_deltas / this->distance_deltas) - (this->distance_deltas / EARTH_CONST);

  // Sweep through each band leaving just a trail of the current max value found.
  // Eg; 2, 4, 1, 3, 5, 7, 2, 3, 1
  // becomes...
  // Eg; 2, 4, 4, 4, 5, 7, 7, 7, 7
  this->angles = af::scan(this->angles, 0, AF_BINARY_MAX);
}

void Viewsheds::findVisiblePoints() {
  // Just so we can get access to the points before and after any given point
  // TODO: Seems unessecary to have to do this, can we use createStridedArray()?
  af::array back_one = af::shift(this->angles, 1);
  af::array back_two = af::shift(this->angles, 2);

  // Find all visible points
  this->visible = back_one != this->angles;
  af::freePinned(&this->angles);
  //af::free(&this->angles);
  // First (PoV) is always visible
  this->visible(0, af::span) = true;

  // Find where ring sectors open
  this->opens = (back_two == back_one) && this->visible;
  // First point is always the start of a ring sector
  this->opens(0, af::span) = true;

  // Find points where ring sectors close
  this->closes = (back_two != back_one) && !this->visible;
  // Last point should close if there's an open ring sector
  this->closes(af::end, af::span) = this->visible(af::end, af::span) == true;
}

void Viewsheds::findRingSectors() {
  af::freePinned(&this->closes);
  af::freePinned(&this->opens);
}

void Viewsheds::sumSurfaces() {
  // Cast visibles to floats, in prep for distance_deltas
  // TODO: Is this the best way to cast. Docs indicate there is only af::cast() for C.
  af::array surfaces = this->visible * 1.0;
  // Replace visible points with distance deltas
  af::replace(surfaces, !this->visible, this->distance_deltas);
  af::freePinned(&this->visible);

  // This normalises surfaces due to repeated visibility over sectors.
  // For example points near the PoV may be counted 180 times giving
  // an unrealistic aggregation of total surfaces. This trig operation
  // considers adjacent points as 1/60th, points that are about 57 cells
  // away as 1 and points futher away as increasingly over 1.
  // Bear in my mind that due to the parallel nature of the Band of Sight
  // distant points may not even be swept for a given Pov, therefore it
  // is an approximation to overcompansenate the surface area of very
  // distant points.
  // Ultimately the reasoning for such normalisation is not so much to
  // measure true surface areas, but to ensure as much as possible that all
  // PoVs are treated consistently so that overlapping raster map tiles
  // do not have visual artefacts.
  surfaces = surfaces * std::tan(TO_RADIANS);

  // Join front and back bands under the same point
  surfaces = af::moddims(surfaces, this->computable_band_size * 2, this->dem.computable_points_count);
  // Sum the surfaces and make 1 dimensional
  surfaces = af::flat(af::sum(surfaces));

  // Get the indexes for the surfaces
  af::array ids = af::moddims(bands, this->computable_band_size, this->computable_bands);
  ids = ids(0, af::span);
  ids = moddims(ids, 2, this->dem.computable_points_count);
  ids = af::flat(ids(0, af::span));

  // Sort the summations so that they represent a 1D vesion of the
  // square found in the middle of the DEM representing the points
  // that can actually have true summations.
  af::array sorted = af::array(this->dem.computable_points_count);
  af::array indices = af::array(this->dem.computable_points_count);
  af::sort(sorted, indices, ids);
  surfaces = surfaces(indices);
  // Finally add this sector's values to the previous sector's
  this->cumulative_surfaces += surfaces;

  // Prevent the JIT from getting too big
  cumulative_surfaces.eval();
  af::sync();
}

void Viewsheds::transferToHost() {
  af_print(cumulative_surfaces);
  this->dem.tvs_complete = this->cumulative_surfaces.host<float>();
}

}  // namespace TVS
