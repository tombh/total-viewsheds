#include "helper.h"

#include "../src/definitions.h"
#include "../src/DEM.h"

#include "fixtures.h"

TEST_CASE("Bands of sight") {
  setup();

  SECTION("Building bands") {
    createMockDEM(fixtures::doublePeakDEM);

    SECTION("sizes are correct") {
      Compute compute = reconstructBandsForSector(0);
      REQUIRE(compute.dem.computable_points_count == 9);
      REQUIRE(compute.bos.band_size == 4);
    }

    SECTION("for Sector 0") {
      std::string bands = reconstructedBandsToASCII(0);
      std::string expected =
        "\n"
        " 57 48 39 30 30 21 12  3\n"
        " 58 49 40 31 31 22 13  4\n"
        " 59 50 41 32 32 23 14  5\n"
        " 66 57 48 39 39 30 21 12\n"
        " 67 58 49 40 40 31 22 13\n"
        " 68 59 50 41 41 32 23 14\n"
        " 75 66 57 48 48 39 30 21\n"
        " 76 67 58 49 49 40 31 22\n"
        " 77 68 59 50 50 41 32 23\n\n";
      REQUIRE(bands == expected);
    }

    SECTION("for Sector 15") {
      std::string bands = reconstructedBandsToASCII(15);
      std::string expected =
        "\n"
        " 49 40 39 30 30 21 20 11\n"
        " 50 41 40 31 31 22 21 12\n"
        " 51 42 41 32 32 23 22 13\n"
        " 58 49 48 39 39 30 29 20\n"
        " 59 50 49 40 40 31 30 21\n"
        " 60 51 50 41 41 32 31 22\n"
        " 67 58 57 48 48 39 38 29\n"
        " 68 59 58 49 49 40 39 30\n"
        " 69 60 59 50 50 41 40 31\n\n";
      REQUIRE(bands == expected);
    }


    SECTION("for Sector 90") {
      std::string bands = reconstructedBandsToASCII(90);
      std::string expected =
        "\n"
        " 33 32 31 30 30 29 28 27\n"
        " 34 33 32 31 31 30 29 28\n"
        " 35 34 33 32 32 31 30 29\n"
        " 42 41 40 39 39 38 37 36\n"
        " 43 42 41 40 40 39 38 37\n"
        " 44 43 42 41 41 40 39 38\n"
        " 51 50 49 48 48 47 46 45\n"
        " 52 51 50 49 49 48 47 46\n"
        " 53 52 51 50 50 49 48 47\n\n";
      REQUIRE(bands == expected);
    }

    SECTION("for Sector 135") {
      std::string bands = reconstructedBandsToASCII(135);
      std::string expected =
        "\n"
        "  6 14 22 30 30 38 46 54\n"
        "  7 15 23 31 31 39 47 55\n"
        "  8 16 24 32 32 40 48 56\n"
        " 15 23 31 39 39 47 55 63\n"
        " 16 24 32 40 40 48 56 64\n"
        " 17 25 33 41 41 49 57 65\n"
        " 24 32 40 48 48 56 64 72\n"
        " 25 33 41 49 49 57 65 73\n"
        " 26 34 42 50 50 58 66 74\n\n";
      REQUIRE(bands == expected);
    }
  }
}
