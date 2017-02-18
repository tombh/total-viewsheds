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
  //   I think it's clear from this that TVS values are calculated relative
  //   to the sector-ordered orientation.
  SECTION("for the double-peaked DEM then values in the dip should be lowest") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_tvs =
      "0.065826 0.140063 0.098749 0.142516 0.127262 \n"
      "0.125182 0.160039 0.142190 0.142351 0.151213 \n"
      "0.129233 0.153524 0.156935 0.157242 0.113736 \n"
      "0.169986 0.145503 0.166869 0.129122 0.131960 \n"
      "0.144563 0.167062 0.128396 0.123784 0.101780 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }
}
