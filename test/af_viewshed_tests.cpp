#include <arrayfire.h>

#include "helper.h"

#include "../src/definitions.h"
#include "../src/Compute.h"
#include "fixtures.h"

Compute computeTVS() {
  Compute compute = Compute();
  compute.forcePreCompute();
  compute.dem.setToCompute();
  compute.dem.prepareForCompute();
  compute.dem.setElevations();
  compute.sector.prepareForCompute();
  compute.sector.changeAngle(0);
  compute.sector.loopThroughBands();
  compute.sector.viewsheds.transferSectorData(compute.sector.bos_manager);
  return compute;
}

TEST_CASE("Arrayfire viewsheds") {
  setup();
  createMockDEM(fixtures::mountainDEM);

  SECTION("Hull pass") {
    Compute compute = computeTVS();
    compute.sector.viewsheds.hullPass();
    float *angles = af::flat(compute.sector.viewsheds.angles).host<float>();
    REQUIRE(angles[0] < -3400000000000000);
    REQUIRE(angles[1] == Approx(-4.00061));
  }

  SECTION("Finding visible points") {
    Compute compute = computeTVS();
    compute.sector.viewsheds.hullPass();
    compute.sector.viewsheds.findVisiblePoints();
    int *visibles = af::flat(compute.sector.viewsheds.visible * 1).host<int>();
    REQUIRE(visibles[0] == 1);
    REQUIRE(visibles[1] == 1);
    REQUIRE(visibles[2] == 0);
  }

  SECTION("Summing surfaces") {
    Compute compute = computeTVS();
    compute.sector.viewsheds.hullPass();
    compute.sector.viewsheds.findVisiblePoints();
    compute.sector.viewsheds.sumSurfaces();
    compute.sector.viewsheds.transferToHost();
    REQUIRE(compute.dem.tvs_complete[0] == Approx(0.0349));
  }
}
