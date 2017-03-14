#include <sstream>

#include <plog/Log.h>
#include <gflags/gflags.h>

#include "definitions.h"
#include "helper.h"

namespace TVS {
namespace helper {

void init(int argc, char *argv[]) {
  plog::init(plog::info, FLAGS_log_file.c_str());
  gflags::ParseCommandLineFlags(&argc, &argv, false); // false prevents stripping of args
  buildPathStrings();
  if (FLAGS_max_line_of_sight == -1) {
    FLAGS_max_line_of_sight = FLAGS_dem_width / 3;
  }
}

void buildPathStrings() {
  ETC_DIR = FLAGS_etc_dir.c_str();
  INPUT_DIR = ETC_DIR + "/" + FLAGS_input_dir;
  OUTPUT_DIR = ETC_DIR + "/" + FLAGS_output_dir;
  INPUT_DEM_FILE = INPUT_DIR + "/" + FLAGS_input_dem_file;
  SECTOR_DIR = OUTPUT_DIR + "/" + FLAGS_sector_dir;
  RING_SECTOR_DIR = OUTPUT_DIR + "/" + FLAGS_ring_sector_dir;
  TVS_RESULTS_FILE = OUTPUT_DIR + "/" + FLAGS_tvs_results_file;
  TVS_PNG_FILE = OUTPUT_DIR + "/" + FLAGS_tvs_png_file;
}

void createDirectory(std::string directory) {
  std::stringstream cmd;
  cmd << "mkdir -p " << directory;
  system(cmd.str().c_str());
}

void createDirectories() {
  createDirectory(ETC_DIR);
  createDirectory(INPUT_DIR);
  createDirectory(OUTPUT_DIR);
  createDirectory(SECTOR_DIR);
  createDirectory(RING_SECTOR_DIR);
}

}
}
