#include "helper.h"

#include "../src/definitions.h"
#include "../src/Sector.h"
#include "../src/DEM.h"

#include "fixtures.h"

/*
 * About the ASCII format used here:
 *   1. Columns are:
 *      'Linked list index', 'DEM index', 'Sector-ordered index',
 *      'distance', 'previous', 'next', 'Special markers'.
 *   2. Special markers as follows:
 *      P: Point of View
 *      F: First (start of a forward sweep)
 *      L: Last (end of a forward sweep)
 *      H: Head (most-recent node to be added)
 *      T: Tail (oldest node to be added)
 *   3. Sometimes H points outside the BoS limits so it is
 *      included seperately as well.

 * Things to remember about a Band of Sight:
 *   1. Sweeping forward uses 'next' links.
 *   2. Sweeping backwards uses 'prev' links.
 *   3. -1 signifies the end of a forward sweep.
 *   4. -2 signifies the end of a backward sweep.
**/
TEST_CASE("Bands of sight") {
  setup();

  SECTION("5x5 DEM") {
    DEM dem = DEM();
    Sector sector = Sector(dem);

    // Note that in the first sector, the dem and sector indexes
    // are exactly the same.
    SECTION("Sector 0"){
      SECTION("BoS setup"){
        computeBOSFor(&sector, 0, 0, "sector-indexed");
        std::string expected =
          "0 20 20 4.00000  -2 -1 PFLT\n"
          "H=1\n";
        std::string bos = bosToASCII(&sector);
        REQUIRE(bos == expected);
      }

      SECTION("First iteration, nodes 15 & 10 added"){
        computeBOSFor(&sector, 0, 1, "sector-indexed");
        std::string expected =
          "0 20 20 4.00000   1 -1 PLT\n"
          "1 15 15 3.00000   2  0 \n"
          "2 10 10 2.00000  -2  1 F\n"
          "H=3\n";
        std::string bos = bosToASCII(&sector);
        REQUIRE(bos == expected);
      }

      SECTION("2nd iteration: first full BoS"){
        computeBOSFor(&sector, 0, 2, "sector-indexed");
        std::string expected =
          "0 20 20 4.00000   1 -1 LHT\n"
          "1 15 15 3.00000   2  0 P\n"
          "2 10 10 2.00000   3  1 \n"
          "3 5  5  1.00000   4  2 \n"
          "4 0  0  0.00000  -2  3 F\n"
          "H=0\n";
        std::string bos = bosToASCII(&sector);
        REQUIRE(bos == expected);
      }

      SECTION("3rd iteration: one step only adds one new node"){
        computeBOSFor(&sector, 0, 3, "sector-indexed");
        std::string expected =
          "0 21 21 4.00002   1 -1 L\n"
          "1 15 15 3.00000   2  0 HT\n"
          "2 10 10 2.00000   3  1 P\n"
          "3 5  5  1.00000   4  2 \n"
          "4 0  0  0.00000  -2  3 F\n"
          "H=1\n";
        std::string bos = bosToASCII(&sector);
        REQUIRE(bos == expected);
      }

    }

    SECTION("Sector 55"){
      SECTION("DEM point 12: printed order different from link-following order") {
        computeBOSFor(&sector, 55, 12, "dem-indexed");
        std::string expected =
          "0 0  0  0.00000  -2  3 F\n"
          "1 18 20 4.17817   2  4 HT\n"
          "2 12 12 2.78545   3  1 P\n"
          "3 6  4  1.39272   0  2 \n"
          "4 19 23 4.99733   1 -1 L\n"
          "H=1\n";
        std::string bos = bosToASCII(&sector);
        REQUIRE(bos == expected);
      }
    }

    SECTION("Sector 90"){
      SECTION("DEM point 0: BoS does not fall inside DEM") {
        computeBOSFor(&sector, 90, 0, "dem-indexed");
        std::string expected = "H=0\n";
        std::string bos = bosToASCII(&sector);
        REQUIRE(bos == expected);
      }
    }
  }

  SECTION("6x6 DEM"){
    FLAGS_dem_width = 6;
    FLAGS_dem_height = 6;
    DEM dem = DEM();
    Sector sector = Sector(dem);

    SECTION("Looping through all points doesn't produce an error") {
      computeBOSFor(&sector, 90, 35, "sector-indexed");
      std::string expected =
        "0 0  5  0.00000  -2 -1 FLT\n"
        "H=1\n";
      std::string bos = bosToASCII(&sector);
      REQUIRE(bos == expected);
    }
  }

  SECTION("Compressing bands") {
    createMockDEM(fixtures::doublePeakDEM);

    SECTION("RLE compression is correct") {
      Compute compute = reconstructBandsForSector(0);
      REQUIRE(compute.sector.bos_manager.band_data_size == 36);
      // First front band
      REQUIRE(compute.sector.bos_manager.band_markers[0] == 8);
      REQUIRE(compute.sector.bos_manager.band_data[8 * 2] == 3);
      REQUIRE(compute.sector.bos_manager.band_data[(8 * 2) + 1] == 9);
      // First back band
      REQUIRE(compute.sector.bos_manager.band_markers[9] == 10);
      REQUIRE(compute.sector.bos_manager.band_data[9 * 2] == 3);
      REQUIRE(compute.sector.bos_manager.band_data[(9 * 2) + 1] == -9);
      // Last front band
      REQUIRE(compute.sector.bos_manager.band_markers[8] == 24);
      REQUIRE(compute.sector.bos_manager.band_data[8 * 2] == 3);
      REQUIRE(compute.sector.bos_manager.band_data[(8 * 2) + 1] == 9);
      // Last back band
      REQUIRE(compute.sector.bos_manager.band_markers[17] == 26);
      REQUIRE(compute.sector.bos_manager.band_data[9 * 2] == 3);
      REQUIRE(compute.sector.bos_manager.band_data[(9 * 2) + 1] == -9);
    }
  }

  SECTION("Building bands") {
    createMockDEM(fixtures::doublePeakDEM);

    SECTION("sizes are correct") {
      Compute compute = reconstructBandsForSector(0);
      REQUIRE(compute.dem.computable_points_count == 9);
      REQUIRE(compute.sector.bos_manager.computable_band_size == 4);
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
        " 14 13 22 30 30 38 46 54\n"
        "  6 15 23 31 31 39 47 55\n"
        "  8 16 24 32 32 40 48 56\n"
        " 14 23 31 39 39 47 55 63\n"
        " 16 24 32 40 40 48 56 64\n"
        " 17 25 33 41 41 49 57 65\n"
        " 24 32 40 48 48 56 64 72\n"
        " 25 33 41 49 49 57 65 73\n"
        " 25 34 42 50 50 58 66 74\n\n";
      REQUIRE(bands == expected);
    }
  }

}
