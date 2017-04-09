#include <stdio.h>
#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../src/Axes.h"
#include "../src/DEM.h"
#include "../src/definitions.h"
#include "../src/helper.h"
#include "helper.h"

// Create a fake DEM for testing purposes
void createMockDEM(const int dem[]) {
  FLAGS_max_line_of_sight = 3;
  FLAGS_dem_scale = 1;
  FLAGS_dem_width = 9;
  FLAGS_dem_height = 9;
  int row_step, col_step, i;
  FILE *f;
  f = fopen(INPUT_DEM_FILE.c_str(), "wb");
  // Skip the header as if it were there
  fseek(f, 256, SEEK_SET);

  if (f == NULL) {
    throw "Error opening mock DEM, tests should be run from the base "
          "project path.";
  } else {
    // Convert to .bt format
    for (int x = 0; x < FLAGS_dem_width; x++) {
      row_step = (FLAGS_dem_width * FLAGS_dem_width) + x;
      for (int y = 0; y < FLAGS_dem_width; y++) {
        col_step = (y + 1) * FLAGS_dem_width;
        i = row_step - col_step;
        fwrite(&dem[i], 2, 1, f);
      }
    }
  }
  fclose(f);
}

// System command with output
std::string exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != NULL) result += buffer.data();
  }
  return result;
}

void setFLAGDefaults() {
  FLAGS_dem_scale = 1;
  FLAGS_dem_width = 5;
  FLAGS_dem_height = 5;
}

void emptyDirectories() {
  std::stringstream cmd;
  if (std::string(ETC_DIR).find(".") != 0) {
    throw "Not deleting unexpected ETC_DIR path";
  }
  cmd << "rm -rf " << ETC_DIR;
  exec(cmd.str().c_str());
}

void setup() {
  setFLAGDefaults();
  emptyDirectories();
  helper::createDirectories();
}

void tearDown() { }

void computeDEMFor(DEM *dem, int angle) {
  Axes axes = Axes(*dem);
  axes.adjust(angle);
}

std::string sectorOrderedDEMPointsToASCII(DEM *dem) {
  std::stringstream out;
  int inverted[dem->size];
  for (int point = 0; point < dem->size; point++) {
    inverted[dem->sector_ordered[point]] = point;
  }
  for (int point = 0; point < dem->size; point++) {
    out << std::right << std::setw(3) << std::setfill(' ') << std::to_string(inverted[point]);
    if ((point % dem->width) == (dem->width - 1)) out << "\n";
  }
  out << "\n";
  return out.str();
}

std::string sightOrderedDEMPointsToASCII(DEM *dem) {
  std::stringstream out;
  for (int point = 0; point < dem->size; point++) {
    out << std::right << std::setw(3) << std::setfill(' ') << std::to_string(dem->sight_ordered[point]);
    if ((point % dem->width) == (dem->width - 1)) out << "\n";
  }
  out << "\n";
  return out.str();
}

std::string nodeDistancesToASCII(DEM *dem) {
  std::stringstream out;
  for (int point = 0; point < dem->size; point++) {
    out << std::to_string(dem->distances[point]) << " ";
    if ((point % dem->width) == (dem->width - 1)) out << "\n";
  }
  out << "\n";
  return out.str();
}

void computeBOSFor(BOS *bos, int angle) {
  Axes axes = Axes(bos->dem);
  axes.adjust(angle);
  bos->dem.setToPrecompute();
  bos->saveBandData(angle);
}

Compute reconstructBandsForSector(int angle) {
  {
    Compute compute = Compute();
    compute.preCompute();
  }
  Compute compute = Compute();
  compute.dem.setToCompute();
  compute.changeAngle(angle);
  compute.bos.loadBandData(angle);
  return compute;
}

std::string reconstructedBandsToASCII(int angle) {
  int front_dem_id, back_dem_id;
  std::stringstream out;
  Compute compute = reconstructBandsForSector(angle);
  int band_size = compute.bos.band_size;
  int points = compute.dem.computable_points_count;

  out << "\n";
  for (int point = 0; point < points; point++) {
    std::vector<int> front_band;
    std::vector<int> back_band;
    front_dem_id = back_dem_id = compute.dem.tvsIdToPOVId(point);
    // Build the bands
    for (int band_point = 0; band_point < band_size; band_point++) {
      front_band.push_back(front_dem_id);
      back_band.push_back(back_dem_id);
      front_dem_id += compute.bos.band_deltas[band_point];
      back_dem_id -= compute.bos.band_deltas[band_point];
    }
    // 'Render' the bands
    for (int band_point = band_size - 1; band_point >= 0; band_point--) {
      out << std::right << std::setw(3) << std::setfill(' ') << back_band[band_point];
    }
    for (int band_point = 0; band_point < band_size; band_point++) {
      out << std::right << std::setw(3) << std::setfill(' ') << front_band[band_point];
    }
    out << "\n";
  }
  out << "\n";
  return out.str();
}

