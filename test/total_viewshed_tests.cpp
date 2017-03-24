#include "helper.h"

#include "../src/definitions.h"
#include "../src/Compute.h"
#include "../src/Output.h"

std::string calculateTVS() {
  {
    Compute compute = Compute();
    compute.forcePreCompute();
  }
  Compute compute = Compute();
  compute.forceCompute();
  Output output(compute.dem);
  return output.tvsToASCII();
}

TEST_CASE("Total viewsheds") {
  setup();

  SECTION("for the mountain DEM TVS values should be greater in the middle") {
    createMockDEM(fixtures::mountainDEM);
    std::string expected_tvs =
      "32.695278 21.426254 32.695290 \n"
      "20.698776 40.228973 20.686457 \n"
      "32.028805 19.958977 32.028824 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }

  SECTION("for the double-peaked DEM then values in the dip should be lowest") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_tvs =
      "35.602600 31.393995 30.822342 \n"
      "29.533701 37.877922 22.686573 \n"
      "30.406879 21.737001 18.455341 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }
}

