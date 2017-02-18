#include "helper.h"

#include "../src/definitions.h"
#include "../src/Compute.h"
#include "../src/Output.h"

// Using a md5sum isn't really cool for testing because it doesn't
// indicate what went wrong if there's a mismatch. Hopefully the
// things that could go wrong are covered by other tests.
SCENARIO("Total Viewshed PNG output") {
  std::stringstream cmd;
  std::stringstream expected;
  std::string current_md5_from_png = "30926abf1c74a2c59c7ec3a3deb77571";

  setup();
  createMockDEM(fixtures::mountainDEM);
  Compute compute = Compute();
  compute.forcePreCompute();
  compute.forceCompute();
  Output output = Output(compute.dem);
  output.tvsToPNG();

  cmd << "md5sum " << TVS_PNG_FILE;
  expected << current_md5_from_png << "  " << TVS_PNG_FILE << "\n";
  std::string md5sum = exec(cmd.str().c_str());

  REQUIRE(md5sum == expected.str());
}
