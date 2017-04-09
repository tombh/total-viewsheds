#include "helper.h"

#include "../src/Axes.h"
#include "../src/Compute.h"
#include "../src/Output.h"
#include "../src/definitions.h"

SCENARIO("Rotated axes for sectors") {
  setup();
  DEM dem = DEM();
  dem.setToPrecompute();

  // Zero degrees still gets a 'shift angle' applied to
  // prevent point alignments.
  GIVEN("A rotation of 0 degrees"){
    computeDEMFor(&dem, 0);

    // Ordering from an axis a bit like:
    //
    //  |
    //  |
    //   |
    //   |
    //    |
    //    |
    //
    THEN("Sector-based ordering should be correct") {
      std::string orthArray = sectorOrderedDEMPointsToASCII(&dem);
      std::string expected =
        "  4  9 14 19 24\n"
        "  3  8 13 18 23\n"
        "  2  7 12 17 22\n"
        "  1  6 11 16 21\n"
        "  0  5 10 15 20\n\n";
      REQUIRE(orthArray == expected);
    }

    // Ordering from an axis a bit like
    //
    // ........~~~~~~~~````````
    //
    THEN("Sight-based ordering should be correct") {
      std::string orthArray = sightOrderedDEMPointsToASCII(&dem);
      std::string expected =
        "  0  1  2  3  4\n"
        "  5  6  7  8  9\n"
        " 10 11 12 13 14\n"
        " 15 16 17 18 19\n"
        " 20 21 22 23 24\n\n";
      REQUIRE(orthArray == expected);
    }

    THEN("Distances should be correct") {
      std::string distances = nodeDistancesToASCII(&dem);
      std::string expected =
        "0.000000 0.000017 0.000035 0.000052 0.000070 \n"
        "1.000000 1.000017 1.000035 1.000052 1.000070 \n"
        "2.000000 2.000017 2.000035 2.000052 2.000070 \n"
        "3.000000 3.000017 3.000035 3.000052 3.000070 \n"
        "4.000000 4.000018 4.000035 4.000052 4.000070 \n\n";
      REQUIRE(distances == expected);
    }
  }

  GIVEN("A rotation of 135 degrees"){
    computeDEMFor(&dem, 135);

    // Ordering from an axis a bit like:
    //
    //               .~`
    //            .~`
    //         .~`
    //      .~`
    //   .~`
    //
    THEN("Sector-based ordering should be correct") {
      std::string orthArray = sectorOrderedDEMPointsToASCII(&dem);
      std::string expected =
        " 24 22 19 15 10\n"
        " 23 20 16 11  6\n"
        " 21 17 12  7  3\n"
        " 18 13  8  4  1\n"
        " 14  9  5  2  0\n\n";
      REQUIRE(orthArray == expected);
    }

    // Ordering from an axis a bit like:
    //
    //  `~.
    //     `~.
    //        `~.
    //           `~.
    //              `~.
    //
    THEN("Sight-based ordering should be correct") {
      std::string orthArray = sightOrderedDEMPointsToASCII(&dem);
      std::string expected =
        " 14 18 21 23 24\n"
        "  9 13 17 20 22\n"
        "  5  8 12 16 19\n"
        "  2  4  7 11 15\n"
        "  0  1  3  6 10\n\n";
      REQUIRE(orthArray == expected);
    }

    THEN("Distances should be correct") {
      std::string distances = nodeDistancesToASCII(&dem);
      std::string expected =
        "0.000000 0.707094 1.414189 2.121283 2.828378 \n"
        "-0.707119 -0.000025 0.707070 1.414164 2.121259 \n"
        "-1.414238 -0.707144 -0.000049 0.707045 1.414140 \n"
        "-2.121357 -1.414263 -0.707168 -0.000074 0.707020 \n"
        "-2.828476 -2.121382 -1.414288 -0.707193 -0.000099 \n\n";
      REQUIRE(distances == expected);
    }
  }
}
