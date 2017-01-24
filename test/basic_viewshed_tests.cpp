#include "helper.h"

#include "../src/DEM.h"
#include "../src/Output.h"
#include "fixtures.h"

// Here we use a simple ASCII representation of the opening and closing of ring
// sectors.
// A '.' can be either inside or outisde a ring sector.
// A '+' is the opening and a '-' is the closing.
// Eg; Given this ASCII art profile of 2 mountain peaks, where the observer is
// 'X':
//
//                     .*`*.
//                  .*`     `*.
//       .      X.*`           `*.
//    .*` `*. .*`                 `*.
// .*`       `                      `*.
//
// there would be 2 ring sectors, both opening at the same point but looking
// in different directions:
//
// .....-.......+........-.............
//
// Or to use 0s and 1s to show the surfaces seen by the observer:
//
// 000001111111111111111110000000000000

std::string calculateViewshedFor(int viewer) {
  setup();
  DEM dem = DEM();
  Output output = Output(dem);
  dem.preCompute();
  dem.compute();
  return output.viewshedToASCII(viewer);
}

SCENARIO("Basic viewsheds") {
  GIVEN("A DEM with a symmetrical mountain summit in the middle") {
    createMockDEM(fixtures::mountainDEM);

    THEN("a viewer on the mountain summit should see everything") {
      std::string expected_viewshed =
          "- - - - - \n"
          "- . . . - \n"
          "- . + . - \n"
          "- . . . - \n"
          "- - - - - \n";
      std::string result = calculateViewshedFor(12);
      REQUIRE(result == expected_viewshed);
    }

    THEN(
        "the mountain should prevent a viewer in a corner seeing the far "
        "corner") {
      std::string expected_viewshed =
          "+ . . . . \n"
          ". . - - - \n"
          ". - . - . \n"
          ". - - . . \n"
          ". - . . . \n";
      std::string result = calculateViewshedFor(0);
      REQUIRE(result == expected_viewshed);
    }
  }

  GIVEN("A double peaked DEM") {
    createMockDEM(fixtures::doublePeakDEM);
    THEN("a viewer in the top-left corner should see 2 ring sectors") {
      // Point 24 doesn't have a closing marker, not sure why?
      // Maybe because ring sectors aren't exactly the same concept as visible
      // points?
      std::string expected_viewshed =
          "+ . - . . \n"
          ". . - . . \n"
          "- - - + - \n"
          ". . + . - \n"
          ". . - - . \n";
      std::string result = calculateViewshedFor(0);
      REQUIRE(result == expected_viewshed);
    }
  }
}
