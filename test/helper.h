#include <iostream>

#include <plog/Log.h>
#include <catch/catch.hpp>

#include "fixtures.h"
#include "../src/Compute.h"
#include "../src/DEM.h"
#include "../src/Sector.h"

#ifndef TVS_TEST_HELPER_H
#define TVS_TEST_HELPER_H

namespace TVS {};
using namespace TVS;

void createMockDEM(const int dem[]);
std::string exec(const char *cmd);
void setup();
void tearDown();
void computeDEMFor(DEM*, int);
void computeBOSFor(Sector*, int, int, std::string);
void bosLoopToDEMPoint(Sector*, int);
void bosLoopToSectorPoint(Sector*, int, bool);
std::string sectorOrderedDEMPointsToASCII(DEM*);
std::string sightOrderedDEMPointsToASCII(DEM*);
std::string nodeDistancesToASCII(DEM*);
std::string bosToASCII(Sector*);
std::string sweepToASCII(Sector*, std::string);
Compute computeFullBOSForSector(int);
void computeSweepFor(Compute*, std::string, int, int);

#endif
