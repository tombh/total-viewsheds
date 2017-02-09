#include <plog/Log.h>
#include <gflags/gflags.h>

#include "definitions.h"
#include "helper.h"
#include "DEM.h"
#include "Output.h"

int main(int argc, char *argv[]) {
  TVS::helper::init(argc, argv);

  printf("Logging to %s...\n", TVS::FLAGS_log_file.c_str());

  TVS::DEM dem = TVS::DEM();
  TVS::Output output = TVS::Output(dem);

  if(!TVS::FLAGS_is_precompute) {
    printf("Computing (using precomputed data in %s)...\n", TVS::SECTOR_DIR.c_str());
  } else {
    printf("Precomputing...\n", dem.size);
  }

  dem.compute();

  printf("%d DEM points computed\n", dem.size);

  if(!TVS::FLAGS_is_precompute) {
    output.tvsToPNG();
    printf("%s written\n", TVS::TVS_PNG_FILE.c_str());
  }

  exit(0);
}
