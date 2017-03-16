#include <plog/Log.h>
#include <gflags/gflags.h>

#include "definitions.h"
#include "helper.h"
#include "Compute.h"

int main(int argc, char *argv[]) {
  TVS::helper::init(argc, argv);

  printf("Logging to %s...\n", TVS::FLAGS_log_file.c_str());

  if(TVS::FLAGS_run_benchmarks) {
    LOGI << "Running Portishead benchmark...";
    TVS::INPUT_DEM_FILE = "./portishead-benchmark.bt";
    TVS::TVS_RESULTS_FILE = TVS::OUTPUT_DIR + "/benchmark.bt";
    TVS::FLAGS_dem_width = 168;
    TVS::FLAGS_dem_height = 168;
    TVS::FLAGS_max_line_of_sight = (168 * 30) / 3;
    TVS::Compute compute = TVS::Compute();
    compute.forcePreCompute();
    compute.forceCompute();
    compute.output();
    exit(0);
  }

  TVS::Compute compute = TVS::Compute();

  if(!TVS::FLAGS_is_precompute) {
    printf("Computing (using precomputed data in %s)...\n", TVS::SECTOR_DIR.c_str());
  } else {
    printf("Precomputing %d points...\n", compute.dem.size);
  }

  compute.run();

  printf("%d DEM points computed\n", compute.dem.size);

  if(!TVS::FLAGS_is_precompute) {
    compute.output();
    printf("%s written\n", TVS::TVS_RESULTS_FILE.c_str());
  }

  exit(0);
}
