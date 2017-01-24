#include <iostream>

#include <plog/Log.h>
#include <catch/catch.hpp>

#include "fixtures.h"

#ifndef TVS_TEST_HELPER_H
#define TVS_TEST_HELPER_H

namespace TVS {};
using namespace TVS;

void createMockDEM(const int dem[]);
void setup();
void tearDown();

#endif
