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

  // PROBLEMS:
  // * Why aren't these values perfectly symmetrical along all axes?
  // * I assume that the highest value (~0.16) is some factor of 4. Eg; Being
  //   able to see the whole DEM counts around 4x4 (16) DEM points. Perhaps
  //   it's a factor of 4 and not 5 because the point of the viewer is not
  //   counted in Band of Sight totals?
  SECTION("for the mountain DEM TVS values should be greater in the middle") {
    createMockDEM(fixtures::mountainDEM);
    std::string expected_tvs =
      "0.086926 0.093598 0.069163 0.094122 0.088714 \n"
      "0.081642 0.122147 0.135216 0.122452 0.080595 \n"
      "0.083726 0.138489 0.156935 0.138620 0.084598 \n"
      "0.102320 0.129067 0.141893 0.129372 0.102058 \n"
      "0.105274 0.089578 0.099160 0.089316 0.107063 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }

  // PROBLEMS:
  //   The DEM is symmetrical from top left to bottom right, so why isn't this?
  SECTION("for the double-peaked DEM then values in the dip should be lowest") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_tvs =
      "0.065826 0.116027 0.114567 0.120155 0.114571 \n"
      "0.115825 0.156906 0.142190 0.138883 0.151213 \n"
      "0.148395 0.153524 0.156935 0.157242 0.118891 \n"
      "0.140732 0.145503 0.166869 0.129122 0.133801 \n"
      "0.132483 0.181556 0.115371 0.134210 0.101780 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }
}
