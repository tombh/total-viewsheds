#include "helper.h"

#include "../src/definitions.h"
#include "../src/Compute.h"
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
  Compute compute = Compute();
  compute.forcePreCompute();
  compute.forceCompute();
  Output output = Output(compute.dem);
  return output.viewshedToASCII(viewer);
}

TEST_CASE("Basic viewsheds") {
  setup();

  SECTION("A DEM with a symmetrical mountain summit in the middle") {
    createMockDEM(fixtures::mountainDEM);

    SECTION("Viewer on mountain summit") {
      std::string expected_viewshed =
        ". . . . . . . . . \n"
        ". - ± . ± . ± . . \n"
        ". ± ± ± ± ± ± ± . \n"
        ". ± ± . . . ± ± . \n"
        ". ± ± . o . ± ± . \n"
        ". ± ± . . . ± . . \n"
        ". ± ± ± ± ± ± ± . \n"
        ". . ± ± ± ± ± - . \n"
        ". . . . . . . . . \n";
      std::string result = calculateViewshedFor(40);
      REQUIRE(result == expected_viewshed);
    }

    SECTION("Viewer in the top left of the TVS grid") {
      std::string expected_viewshed =
        ". ± . . . ± . . . \n"
        "± ± ± ± ± ± . . . \n"
        ". ± . . . ± ± . . \n"
        ". ± . o . ± ± . . \n"
        ". ± . . . ± ± . . \n"
        "± ± ± ± ± . ± . . \n"
        ". . ± ± ± ± ± . . \n"
        ". . . . . . . . . \n"
        ". . . . . . . . . \n";
      std::string result = calculateViewshedFor(30);
      REQUIRE(result == expected_viewshed);
    }
  }

  SECTION("A double peaked DEM") {
    createMockDEM(fixtures::doublePeakDEM);
    std::string expected_viewshed =
      "± ± ± ± ± ± ± . . \n"
      "± . . . . ± ± . . \n"
      "± . . . . ± ± . . \n"
      "± . . o . ± . . . \n"
      "± . . . . ± . . . \n"
      "± ± ± ± ± . ± . . \n"
      "± ± ± . ± ± ± . . \n"
      ". . . . . . . . . \n"
      ". . . . . . . . . \n";
    std::string result = calculateViewshedFor(30);
    REQUIRE(result == expected_viewshed);
  }
}
