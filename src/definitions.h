#include <gflags/gflags.h>

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

namespace TVS {

DECLARE_bool(is_precompute);
DECLARE_string(etc_dir);
DECLARE_string(input_dir);
DECLARE_string(output_dir);
DECLARE_string(sector_dir);
DECLARE_string(ring_sector_dir);
DECLARE_string(log_file);
DECLARE_string(input_dem_file);
DECLARE_string(tvs_results_file);
DECLARE_bool(run_benchmarks);
DECLARE_int32(total_sectors);
DECLARE_int32(dem_width);
DECLARE_int32(dem_height);
DECLARE_double(dem_scale);
DECLARE_double(observer_height);
DECLARE_double(sector_shift);
DECLARE_int32(band_size);
DECLARE_int32(reserved_ring_space);
DECLARE_int32(max_line_of_sight);
DECLARE_int32(cl_device);

extern std::string ETC_DIR;
extern std::string INPUT_DIR;
extern std::string OUTPUT_DIR;
extern std::string SECTOR_DIR;
extern std::string RING_SECTOR_DIR;
extern std::string INPUT_DEM_FILE;
extern std::string TVS_RESULTS_FILE;
extern const double TO_RADIANS;
extern const float PI_F;
extern const int EARTH_CONST;

}

#endif
