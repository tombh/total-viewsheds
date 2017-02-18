#include "helper.h"
#include "fixtures.h"

#include "../src/Compute.h"
#include "../src/Sector.h"
#include "../src/DEM.h"

TEST_CASE("Sweeping bands of sights") {
  setup();

  SECTION("For the mountain DEM"){
    createMockDEM(fixtures::mountainDEM);

    // Looking down from the top of the mountain to the bottom right
    SECTION("Sector 0, point 12, forward") {
      Compute compute = Compute();
      computeSweepFor(&compute, "forward", 0, 12);
      std::string sweep = sweepToASCII(&compute.sector, "forward");
      std::string expected =
        "12 0.9 \n"
        "17 0.5 \n"
        "23 0   \n";
      int ring_sectors = compute.sector.nrsF;
      int opening_point = compute.sector.rsF[0][0];
      int closing_point = compute.sector.rsF[0][1];
      REQUIRE(sweep == expected);
      REQUIRE(ring_sectors == 1);
      REQUIRE(opening_point == 12);
      REQUIRE(closing_point == 23);
    }

    // Looking down from the top of the mountain to the top middle
    SECTION("Sector 0, point 12, backward") {
      Compute compute = Compute();
      computeSweepFor(&compute, "backward", 0, 12);
      std::string expected =
        "12 0.9 \n"
        "7  0.5 \n"
        "2  0   \n";
      std::string sweep = sweepToASCII(&compute.sector, "backward");
      int ring_sectors = compute.sector.nrsB;
      int opening_point = compute.sector.rsB[0][0];
      int closing_point = compute.sector.rsB[0][1];
      REQUIRE(sweep == expected);
      REQUIRE(ring_sectors == 1);
      REQUIRE(opening_point == 12);
      REQUIRE(closing_point == 2);
    }

    // Looking from the top left to the bottom right, then the
    // peak should obscure the far corner.
    SECTION("Sector 0, point 0, forward") {
      Compute compute = Compute();
      computeSweepFor(&compute, "forward", 0, 1 );
      std::string sweep = sweepToASCII(&compute.sector, "forward");
      std::string expected =
        "1  0   \n"
        "6  0.5 \n"
        "12 0.9 \n"
        "17 0.5 \n"
        "22 0   \n";
      int ring_sectors = compute.sector.nrsF;
      int opening_point = compute.sector.rsF[0][0];
      int closing_point = compute.sector.rsF[0][1];
      REQUIRE(sweep == expected);
      REQUIRE(ring_sectors == 1);
      REQUIRE(opening_point == 1);
      REQUIRE(closing_point == 17);
    }

    SECTION("Surface checks"){
      SECTION("Sector 90, point 0") {
        Compute compute = Compute();
        computeSweepFor(&compute, "forward", 90, 0);
        REQUIRE(compute.sector.sur_dataF == 0.0f);
      }
    }
  }

  SECTION("For the double-peaked DEM"){
    createMockDEM(fixtures::doublePeakDEM);

    // Looking from top-left to bottom-right
    SECTION("Sector 0, point 0, forward") {
      Compute compute = Compute();
      computeSweepFor(&compute, "forward", 40, 0);
      std::string sweep = sweepToASCII(&compute.sector, "forward");
      std::string expected =
        "0  0   \n"
        "6  0.3 \n"
        "12 0   \n"
        "18 0.7 \n"
        "23 0.9 \n";
      int ring_sectors = compute.sector.nrsF;
      int opening_point1 = compute.sector.rsF[0][0];
      int closing_point1 = compute.sector.rsF[0][1];
      int opening_point2 = compute.sector.rsF[1][0];
      int closing_point2 = compute.sector.rsF[1][1];
      REQUIRE(sweep == expected);
      REQUIRE(ring_sectors == 2);
      REQUIRE(opening_point1 == 0);
      REQUIRE(closing_point1 == 12);
      REQUIRE(opening_point2 == 18);
      REQUIRE(closing_point2 == 23);
    }
  }
}
