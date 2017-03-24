#include "helper.h"

#include "../src/Compute.h"
#include "../src/definitions.h"

TEST_CASE("DEM") {
  setup();
  FLAGS_dem_width = 9;
  FLAGS_dem_height = 9;
  FLAGS_max_line_of_sight = 3;

  SECTION("PoV ID to TVS ID") {
    Compute compute = Compute();
    compute.forcePreCompute();
    int tvs_id = compute.dem.povIdToTVSId(30);
    REQUIRE(tvs_id == 0);
  }

  SECTION("TVS ID to PoV ID") {
    Compute compute = Compute();
    compute.forcePreCompute();
    int tvs_id = compute.dem.tvsIdToPOVId(0);
    REQUIRE(tvs_id == 30);
  }

}
