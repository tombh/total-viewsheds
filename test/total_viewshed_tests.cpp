#include "helper.h"

#include "../src/definitions.h"
#include "../src/Compute.h"
#include "../src/Output.h"

std::string calculateTVS() {
  {
    Compute compute = Compute();
    compute.preCompute();
  }
  Compute compute = Compute();
  compute.compute();
  Output output(compute.dem);
  return output.tvsToASCII();
}

TEST_CASE("Total viewsheds") {
  setup();

  SECTION("for the mountain DEM TVS values should be greater in the middle") {
    createMockDEM(fixtures::mountainDEM);
    std::string expected_tvs =
      "29.574253 18.924240 29.574259 \n"
      "18.924236 34.903873 18.924236 \n"
      "29.574257 18.924240 29.574253 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }

  SECTION("for the double-peaked DEM then values in the dip should be lowest") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_tvs =
      "31.377239 27.208487 28.191574 \n"
      "26.828491 34.046097 22.781456 \n"
      "27.487000 21.809652 18.484119 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }
}

