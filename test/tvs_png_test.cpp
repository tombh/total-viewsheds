#include "helper.h"

#include "../src/definitions.h"
#include "../src/DEM.h"
#include "../src/Output.h"

// Using a md5sum isn't really cool for testing because it doesn't
// indicate what went wrong if there's a mismatch. Hopefully the
// things that could go wrong are covered by other tests.
SCENARIO("Total Viewshed PNG output") {
  std::stringstream cmd;
  std::stringstream expected;
  std::string current_md5_from_png = "48c285f34dce7039b4e037dcdc3c93fd";

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
