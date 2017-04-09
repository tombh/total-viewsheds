#include "helper.h"

#include "../src/Compute.h"
#include "../src/Output.h"
#include "../src/definitions.h"
#include "fixtures.h"

Compute computeTVS(int angle) {
  {
    Compute compute = Compute();
    compute.preCompute();
  }
  createMockDEM(fixtures::mountainDEM);
  Compute compute = Compute();
  compute.dem.setToCompute();
  compute.viewsheds.initialise();
  compute.singleSector(angle);
  compute.viewsheds.transferToHost();
  return compute;
}

TEST_CASE("GPU viewsheds") {
  setup();
  createMockDEM(fixtures::mountainDEM);

  SECTION("TVS accumulated surface for point 0") {
    Compute compute = computeTVS(0);
    REQUIRE(compute.dem.tvs_complete[0] == Approx(0.15708));
  }

  SECTION("Ring sectors for middle point in sector 0") {
    std::vector<int> front_band, back_band;
    Compute compute = computeTVS(0);
    Output output = Output(compute.dem);
    std::vector<std::vector<int>> single_band = output.parseSingleSector(0, 40);

    front_band = single_band[0];
    REQUIRE(front_band[0] == 1);  // Indicates there is only 1 ring sector
    REQUIRE(front_band[1] == 40); // opening
    REQUIRE(front_band[2] == 13); // closing

    back_band = single_band[1];
    REQUIRE(back_band[0] == 1);  // Indicates there is only 1
    REQUIRE(back_band[1] == 40); // opening
    REQUIRE(back_band[2] == 67);  // closing
  }

  SECTION("Ring sectors for middle point in sector 15") {
    std::vector<int> front_band, back_band;
    Compute compute = computeTVS(15);
    Output output = Output(compute.dem);
    std::vector<std::vector<int>> single_band = output.parseSingleSector(15, 40);

    front_band = single_band[0];
    REQUIRE(front_band[0] == 1);  // Indicates there is only 1 ring sector
    REQUIRE(front_band[1] == 40); // opening
    REQUIRE(front_band[2] == 21); // closing

    back_band = single_band[1];
    REQUIRE(back_band[0] == 1);  // Indicates there is only 1
    REQUIRE(back_band[1] == 40); // opening
    REQUIRE(back_band[2] == 59);  // closing
  }
}
