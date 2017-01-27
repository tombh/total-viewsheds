#include <stdio.h>
#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../src/definitions.h"
#include "../src/helper.h"
#include "../src/DEM.h"
#include "helper.h"

// Create a fake DEM for testing purposes
void createMockDEM(const int dem[]) {
  FILE* f;
  f = fopen(INPUT_DEM_FILE.c_str(), "wb");

  if (f == NULL) {
    throw "Error opening mock DEM, tests should be run from the base "
          "project path.";
  } else {
    for (int i = 0; i < 25; i++) {
      fwrite(&dem[i], 2, 1, f);
    }
  }
  fclose(f);
}

// System command with output
std::string exec(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != NULL) result += buffer.data();
  }
  return result;
}

void emptyDirectories() {
  std::stringstream cmd;
  if(std::string(ETC_DIR).find(".") != 0){
    throw "Not deleting unexpected ETC_DIR path";
  }
  cmd << "rm -rf " << ETC_DIR;
  exec(cmd.str().c_str());
}

void setup() {
  emptyDirectories();
  helper::createDirectories();
}

void tearDown() {
  emptyDirectories();
}

