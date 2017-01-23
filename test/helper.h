#include <iostream>

#include <catch/catch.hpp>
#include <plog/Log.h>

#include "fixtures.h"

#ifndef TVS_TEST_HELPER_H
#define TVS_TEST_HELPER_H

namespace TVS {};
using namespace TVS;

void createMockDEM(const int dem[]);
void setup();
void tearDown();

#endif
