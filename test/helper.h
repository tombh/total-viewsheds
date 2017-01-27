#include <iostream>

#include <plog/Log.h>
#include <catch/catch.hpp>

#include "fixtures.h"

#ifndef TVS_TEST_HELPER_H
#define TVS_TEST_HELPER_H

namespace TVS {};
using namespace TVS;

void createMockDEM(const int dem[]);
std::string exec(const char *cmd);
void setup();
void tearDown();

#endif
