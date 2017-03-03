#include "helper.h"

#include "../src/definitions.h"
#include "../src/Compute.h"
#include "../src/Output.h"

std::string calculateTVS() {
  Compute compute = Compute();
  compute.forcePreCompute();
  compute.forceCompute();
  Output output(compute.dem);
  return output.tvsToASCII();
}

// TODO: the peaks in both mock DEMs are actually too abrupt and cause points
// lower down to not be seen. That appears to be why the central surfaces here
// are lower, even though the point has higher elevation.
TEST_CASE("Total viewsheds") {
  setup();

  SECTION("for the mountain DEM TVS values should be greater in the middle") {
    createMockDEM(fixtures::mountainDEM);
    std::string expected_tvs =
      "17.397566 27.558777 17.397566 \n"
      "28.929914 12.961548 28.942245 \n"
      "18.452042 30.313377 18.452042 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }

  SECTION("for the double-peaked DEM then values in the dip should be lowest") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_tvs =
      "19.078993 23.416677 22.099308 \n"
      "25.099541 7.442859 31.392956 \n"
      "23.303123 30.952698 33.724258 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }
}

