#include <plog/Log.h>
#include <gflags/gflags.h>

#include "definitions.h"
#include "helper.h"
#include "DEM.h"
#include "Output.h"

int main(int argc, char *argv[]) {
  TVS::helper::init(argc, argv);

  TVS::DEM dem = TVS::DEM();
  TVS::Output output = TVS::Output(dem);

  dem.compute();
  printf("%s", output.tvsToASCII().c_str());

  exit(0);
}
