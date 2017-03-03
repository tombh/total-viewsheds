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

TEST_CASE("Total viewsheds") {
  setup();

  SECTION("for the mountain DEM TVS values should be greater in the middle") {
    createMockDEM(fixtures::mountainDEM);
    std::string expected_tvs =
        "5.972671 5.534035 \n"
        "5.430229 5.666545 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }

  SECTION("for the double-peaked DEM then values in the dip should be lowest") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_tvs =
      "8.085729 6.082164 \n"
      "6.770525 8.155845 \n\n";

    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }
}

