#include "helper.h"

#include "../src/DEM.h"
#include "../src/Output.h"

std::string calculateTVS() {
  setup();
  DEM dem = DEM();
  Output output = Output(dem);
  dem.preCompute();
  dem.compute();
  return output.tvsToASCII();
}

SCENARIO("Total viewsheds") {
  // PROBLEMS:
  // * Why aren't these values perfectly symmetrical?
  // * Why is point 0 (0.21) so much larger than the middle point or the point
  //   on the opposite side?
  // * I assume that the highest value (0.16) is some factor of 4. Eg; Being
  //   able to see the whole DEM counts around 4x4 DEM points. Perhaps it's 4
  //   and not 5 because the point of the viewer is not counted?
  THEN("for the mountain DEM TVS values should be greater in the middle") {
    createMockDEM(fixtures::mountainDEM);
    std::string expected_tvs =
      "0.215940 0.078242 0.098087 0.078242 0.089737 \n"\
      "0.086336 0.141383 0.129477 0.141383 0.086336 \n"\
      "0.105998 0.124606 0.161911 0.124606 0.105998 \n"\
      "0.092023 0.119315 0.138933 0.119315 0.092023 \n"\
      "0.096730 0.071249 0.084306 0.071249 0.096730 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }

  // PROBLEMS:
  // * It feels like TVS values are flipped from the top-left to the
  //   bottom-right?
  THEN("for the double-peaked DEM then values in the dip should be lowest") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_tvs =
      "0.103489 0.138764 0.141864 0.143935 0.116919 \n"\
      "0.138764 0.168379 0.157581 0.142413 0.149566 \n"\
      "0.146592 0.157581 0.161911 0.164589 0.111435 \n"\
      "0.143935 0.142413 0.164589 0.124245 0.126630 \n"\
      "0.116919 0.129820 0.143688 0.114196 0.069153 \n\n";
    std::string result = calculateTVS();
    REQUIRE(result == expected_tvs);
  }
}
