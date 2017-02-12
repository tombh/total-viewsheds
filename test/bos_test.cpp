#include "helper.h"

#include "../src/Sector.h"
#include "../src/DEM.h"

/*
 * About the ASCII format used here:
 *   1. Columns are:
 *     'Linked list index', 'DEM index', 'Sector-ordered index',
 *     'distance', 'previous', 'next', 'Special markers'.
 *   2. Special markers as follows:
 *     P: Point of View
 *     F: First (start of a forward sweep)
 *     L: Last (end of a forward sweep)
 *     H: Head (most-recent node to be added)
 *     T: Tail (oldest node to be added)

 * Things to remember about a Band of Sight:
 *   1. Sweeping forward uses 'next' links.
 *   2. Sweeping backwards uses 'prev' links.
 *   3. -1 signifies the end of a forward sweep.
 *   4. -2 signifies the end of a backward sweep.
**/
TEST_CASE("Bands of sight") {
  setup();
  DEM dem = DEM();
  Sector sector = Sector(dem);

  // Note that in the first sector, the dem and sector indexes
  // are exactly the same.
  SECTION("Sector 0"){
    SECTION("BoS setup"){
      computeBOSFor(&sector, 0, 0, "sector-indexed");
      std::string expected =
        "0 20 20 4.00000  -2 -1 PFLT\n";
      std::string bos = bosToASCII(&sector);
      REQUIRE(bos == expected);
    }

    SECTION("First iteration, nodes 15 & 10 added"){
      computeBOSFor(&sector, 0, 1, "sector-indexed");
      std::string expected =
        "0 20 20 4.00000   1 -1 PLT\n"
        "1 15 15 3.00000   2  0 \n"
        "2 10 10 2.00000  -2  1 F\n";
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
        "4 0  0  0.00000  -2  3 F\n";
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
        "4 0  0  0.00000  -2  3 F\n";
      std::string bos = bosToASCII(&sector);
      REQUIRE(bos == expected);
    }

  }

  SECTION("Sector 55"){
    SECTION("DEM point 12: linked list order different from link following") {
      computeBOSFor(&sector, 55, 12, "dem-indexed");
      std::string expected =
        "0 0  0  0.00000  -2  3 F\n"
        "1 18 20 4.17817   2  4 HT\n"
        "2 12 12 2.78545   3  1 P\n"
        "3 6  4  1.39272   0  2 \n"
        "4 19 23 4.99733   1 -1 L\n";
      std::string bos = bosToASCII(&sector);
      REQUIRE(bos == expected);
    }
  }
}