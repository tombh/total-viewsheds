#include <signal.h>
#include <stdio.h>

#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

#define BACKWARD_HAS_BFD 1
#include <backward-cpp/backward.hpp>

#include <plog/Log.h>

#include "../src/DEM.h"

void graceful_exit(int signal) { exit(1); }

int main(int argc, char* const argv[]) {
  // global setup
  signal(SIGINT, graceful_exit);
  backward::SignalHandling sh;
  plog::init(plog::info, "tvs.log");

  int result = Catch::Session().run(argc, argv);

  // global clean-up...

  return result;
}
