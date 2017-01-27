#define CATCH_CONFIG_RUNNER
#include <catch/catch.hpp>

#define BACKWARD_HAS_BFD 1
#include <backward-cpp/backward.hpp>

#include "../src/definitions.h"
#include "../src/helper.h"

int main(int argc, char *argv[]) {
  backward::SignalHandling sh;

  TVS::helper::init(argc, argv);
  TVS::FLAGS_etc_dir = "./test/etc";
  TVS::helper::buildPathStrings();

  int result = Catch::Session().run(argc, argv);
  return result;
}
