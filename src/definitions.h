#include <math.h>

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

// General output path for caches and complete results
#define INPUT_DIR "./etc/input/"

// File to read DEM data from
#define INPUT_DEM_FILE INPUT_DIR "mockDEM.bil"

// General output path for caches and complete results
#define OUTPUT_DIR "./etc/output/"

// Cache for precomputed sector calculations (everything that can be done
// without DEM heights).
#define SECTOR_DIR OUTPUT_DIR "sectors/"

// Computed results for ring sector defintions. This is actually the basis
// data for fully reconstructing individual viewsheds
#define RING_SECTOR_DIR OUTPUT_DIR "ring_sectors/"

// Total viewshed values for every point in DEM
#define TVS_RESULTS_FILE OUTPUT_DIR "tvs.flt"

// For displaying a PNG version of the total viewshed
#define TVS_PNG_FILE OUTPUT_DIR "tvs.png"

namespace TVS {

// Number of threads to do simultaneous sector calculations in.
const int threads = 12;

// Think of a double-sided lighthouse moving this many times to make a
// full 360 degree sweep. The algorithm looks forward and backward in
// each sector so if moving 1 degree at a time then 180 sectors are needed.
const int TOTAL_SECTORS = 180;

// Digital Elevation Model (DEM) dimensions
const int DEM_WIDTH = 5;
const int DEM_HEIGHT = 5;
const int DEM_SIZE = DEM_WIDTH * DEM_HEIGHT;

// Should individual ring sector data be stored?
const bool IS_STORE_RING_SECTORS = true;

// Size of each point in the DEM
const float DEM_SCALE = 10;

// Distance the observer is from the ground in metres.
const float OBSERVER_HEIGHT = 1.5;
// Scaling all measurements to a single unit of the DEM_SCALE makes calculations
// simpler.
// TODO: I think this could be a problem when considering DEM's without
// curved earth projections??
const double SCALED_OBSERVER_HEIGHT = OBSERVER_HEIGHT / DEM_SCALE;

// Initial angular shift in sector alignment. This avoids DEM point aligments.
// Eg; The first sector without shift looks from A to B, but with shift looks
// from A to somehwere between B and C.
//
//   A.  .  .  .  .B
//    .  .  .  .  .C
//    .  .  .  .  .
//    .  .  .  .  .
//    .  .  .  .  .
//
// I assume this is because of the algorithm's requirement to order points in
// a Band of Sight in terms of their orthogonal distance from the sector's
// central axis. Point alignment may cause too many points to have the same
// orthogonal distance and therefore make ordering difficult.
const int SECTOR_SHIFT = 0.001;

// Number of points inside a Band of Sight.
const int BAND_SIZE = 5;
const int HALF_BAND_SIZE = (BAND_SIZE - 1) / 2;

// For converting degrees to radians
const double TO_RADIANS = (2 * M_PI) / 360;
// Float typed version of PI
const float PI_F = 3.14159265358979f;

// The quality of the gradient used to create the final TVS PNG image
const int SIZE_OF_TVS_PNG_PALETTE = 1024;

}

#endif
