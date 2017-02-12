#include <plog/Log.h>
#include <gflags/gflags.h>

#include "definitions.h"
#include "helper.h"
#include "Compute.h"

int main(int argc, char *argv[]) {
  TVS::helper::init(argc, argv);

  printf("Logging to %s...\n", TVS::FLAGS_log_file.c_str());

  TVS::Compute compute = TVS::Compute();

  if(!TVS::FLAGS_is_precompute) {
    printf("Computing (using precomputed data in %s)...\n", TVS::SECTOR_DIR.c_str());
  } else {
    printf("Precomputing...\n", compute.dem.size);
  }

  compute.run();

  printf("%d DEM points computed\n", compute.dem.size);

  if(!TVS::FLAGS_is_precompute) {
    compute.output();
    printf("%s written\n", TVS::TVS_RESULTS_FILE.c_str());
    printf("%s written\n", TVS::TVS_PNG_FILE.c_str());
  }

  exit(0);
}
