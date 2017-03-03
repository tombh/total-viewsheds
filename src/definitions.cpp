#include <math.h>

#include <gflags/gflags.h>

#include "definitions.h"

namespace TVS {

DEFINE_bool(is_precompute, false, "Only do pre-computations");

DEFINE_string(
  etc_dir,
  "./etc",
  "General output path for caches and completed results."
);

DEFINE_string(
  input_dir,
  "input",
  "General output path for caches and completed results."
);

DEFINE_string(
  output_dir,
  "output",
  "General output path for precomputation caches and complete results"
);

DEFINE_string(
  sector_dir,
  "sectors",
  "Cache for precomputed sector calculations (everything that can be done"
  "without DEM heights)"
);

DEFINE_string(
  ring_sector_dir,
  "ring_sectors",
  "Computed results for ring sector defintions. This is actually the basis"
  "data for fully reconstructing individual viewsheds"
);

DEFINE_string(
  log_file,
  "tvs.log",
  "Log path"
);

DEFINE_string(
  input_dem_file,
  "dem.bt",
  "File to read DEM data from. Should be binary stream of 2 byte unsigned"
  "integers."
);

DEFINE_string(
  tvs_results_file,
  "tvs.bt",
  "Total viewshed values for every point in DEM"
);

DEFINE_string(
  tvs_png_file,
  "tvs.png",
  "For displaying a PNG version of the total viewshed"
);

DEFINE_int32(
  dem_width,
  5,
  "Width of DEM"
);

DEFINE_int32(
  dem_height,
  5,
  "Height of DEM"
);

DEFINE_double(
  dem_scale,
  30,
  "Size of each point in the DEM in meters"
);

DEFINE_int32(
  max_line_of_sight,
  1000,
  "The maximum distance in metres to search for visible points."
  "For a TVS calculation to be truly correct, it must have access"
  "to all the DEM data around it that may possibly be visible to it."
  "However, the further the distances searched the exponentially"
  "greater the computations required. Note that the largest"
  "currently known line of sight int he world is 538km."
);

DEFINE_int32(
  total_sectors,
  180,
  "Think of a double-sided lighthouse moving this many times to make a"
  "full 360 degree sweep. The algorithm looks forward and backward in"
  "each sector so if moving 1 degree at a time then 180 sectors are"
  "needed"
);

DEFINE_bool(
  is_store_ring_sectors,
  false,
  "Should individual ring sector data be stored?"
);

DEFINE_double(
  observer_height,
  1.5,
  "Distance the observer is from the ground in metres"
);

// I assume this is because of the algorithm's requirement to order points in
// a Band of Sight in terms of their orthogonal distance from the sector's
// central axis. Point alignment may cause too many points to have the same
// orthogonal distance and therefore make ordering difficult.
DEFINE_double(
  sector_shift,
  0.001,
  "Initial angular shift in sector alignment. This avoids DEM point aligments."
  "Eg; The first sector without shift looks from A to B, but with shift looks"
  "from A to somehwere between B and C. \n"
  "\n"
  "A.  .  .  .  .B\n"
  " .  .  .  .  .C\n"
  " .  .  .  .  .\n"
  " .  .  .  .  .\n"
  " .  .  .  .  .\n"
);

DEFINE_int32(
  size_of_tvs_png_palette,
  1024,
  "The quality of the gradient used to create the final TVS PNG image"
);

DEFINE_int32(
  cl_device,
  0,
  "The device to use for computations. Eg; see `clinfo` for available devices"
);

// The following are built up from multiple FLAGS_*
std::string ETC_DIR;
std::string INPUT_DIR;
std::string OUTPUT_DIR;
std::string SECTOR_DIR;
std::string RING_SECTOR_DIR;
std::string INPUT_DEM_FILE;
std::string TVS_RESULTS_FILE;
std::string TVS_PNG_FILE;

// For converting degrees to radians
const double TO_RADIANS = (2 * M_PI) / 360;

// Float-type version of PI
const float PI_F = 3.14159265358979f;

// Constant for calculating elevation offset for a curved earth.
// It is the radius of the earth in metres multipled by 2.
const int EARTH_CONST = 6371000 * 2;
}

