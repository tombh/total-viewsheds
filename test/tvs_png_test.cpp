#include "helper.h"

#include "../src/definitions.h"
#include "../src/DEM.h"
#include "../src/Output.h"

SCENARIO("Total Viewshed PNG output") {
  std::stringstream cmd;
  std::stringstream expected;
  std::string current_md5_from_png = "4cbc36a362e380f4916c430619319f8f";

  setup();
  createMockDEM(fixtures::mountainDEM);
  DEM dem = DEM();
  FLAGS_is_precompute = true;
  dem.compute();
  FLAGS_is_precompute = false;
  dem.compute();
  Output output = Output(dem);
  output.tvsToPNG();

  cmd << "md5sum " << TVS_PNG_FILE;
  expected << current_md5_from_png << "  " << TVS_PNG_FILE << "\n";
  std::string md5sum = exec(cmd.str().c_str());

  REQUIRE(md5sum == expected.str());
}
