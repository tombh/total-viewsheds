#include <iostream>

#include <plog/Log.h>
#include <catch/catch.hpp>

#include "fixtures.h"
#include "../src/Compute.h"
#include "../src/DEM.h"

#ifndef TVS_TEST_HELPER_H
#define TVS_TEST_HELPER_H

namespace TVS {};
using namespace TVS;

void createMockDEM(const int dem[]);
std::string exec(const char *cmd);
void setup();
void tearDown();
void computeDEMFor(DEM*, int);
void computeBOSFor(BOS*, int, int, std::string);
std::string sectorOrderedDEMPointsToASCII(DEM*);
std::string sightOrderedDEMPointsToASCII(DEM*);
std::string nodeDistancesToASCII(DEM*);
std::string bosToASCII(BOS*);
std::string sweepToASCII(BOS*, std::string);
Compute reconstructBandsForSector(int);
std::string reconstructedBandsToASCII(int);
std::string bandStructureToASCII(Compute&);

#endif
