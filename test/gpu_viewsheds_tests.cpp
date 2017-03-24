#include "helper.h"

#include "../src/Compute.h"
#include "../src/definitions.h"
#include "fixtures.h"

Compute computeTVS() {
  {
    Compute compute = Compute();
    compute.forcePreCompute();
  }
  Compute compute = Compute();
  compute.dem.setToCompute();
  compute.dem.prepareForCompute();
  compute.sector.prepareForCompute();
  compute.sector.changeAngle(0);
  compute.sector.calculateViewsheds();
  compute.sector.viewsheds.transferToHost();
  return compute;
}

// This is most useful for TDD.
TEST_CASE("GPU viewsheds") {
  setup();
  createMockDEM(fixtures::mountainDEM);

  SECTION("Calculation") {
    Compute compute = computeTVS();
    REQUIRE(compute.dem.tvs_complete[0] == Approx(0.15708));
  }
}
