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
#include "../src/Sector.h"
#include "../src/definitions.h"
#include "../src/helper.h"
#include "helper.h"

// Create a fake DEM for testing purposes
void createMockDEM(const int dem[]) {
  FLAGS_max_line_of_sight = 60;
  FLAGS_dem_width = 6;
  FLAGS_dem_height = 6;
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
    for (int x = 0; x < 6; x++) {
      row_step = (6 * 6) + x;
      for (int y = 0; y < 6; y++) {
        col_step = (y + 1) * 6;
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
  FLAGS_dem_width = 5;
  FLAGS_dem_height = 5;
  FLAGS_is_store_ring_sectors = false;
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

void tearDown() { emptyDirectories(); }

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

void computeBOSFor(Sector *sector, int angle, int point, std::string ordering) {
  Axes axes = Axes(sector->dem);
  axes.adjust(angle);
  sector->dem.setToPrecompute();
  sector->sector_angle = angle;
  sector->bos_manager.setup(angle);
  if (ordering == "dem-indexed") bosLoopToDEMPoint(sector, point);
  if (ordering == "sector-indexed") bosLoopToSectorPoint(sector, point, false);
  if (ordering == "sector-indexed!") bosLoopToSectorPoint(sector, point, true);
  sector->bos_manager.writeAndClose();
}

Compute ComputeFullBOSForSector(int angle) {
  Compute compute = Compute();
  compute.forcePreCompute();
  compute.dem.setToCompute();
  compute.dem.prepareForCompute();
  compute.sector.changeAngle(angle);
  compute.sector.loopThroughBands();
  return compute;
}

void bosLoopToDEMPoint(Sector *sector, int dem_point) {
  for (int i = 0; i < sector->dem.size; i++) {
    sector->bos_manager.adjustToNextPoint(i);
    int idx = sector->bos_manager.bos.LL[sector->bos_manager.pov].dem_id;
    if (idx == dem_point) break;
  }
}

void bosLoopToSectorPoint(Sector *sector, int sector_point, bool is_print_each) {
  if (is_print_each) {
    LOGI << "\n" + bosToASCII(sector);
  }
  for (int i = 0; i < sector_point; i++) {
    sector->bos_manager.adjustToNextPoint(i);
    if (is_print_each) {
      LOGI << "\n" + bosToASCII(sector);
    }
  }
}

std::string bosToASCII(Sector *sector) {
  std::stringstream out;
  out << std::fixed;
  out.precision(5);
  LinkedList::LinkedListNode point;
  for (int i = 0; i < sector->bos_manager.bos.Count; i++) {
    point = sector->bos_manager.bos.LL[i];
    out << std::left << std::setw(2) << i;
    out << std::left << std::setw(3) << point.dem_id;
    out << std::left << std::setw(3) << sector->dem.sight_ordered[point.dem_id];
    out << std::left << std::setw(8) << sector->dem.distances[point.dem_id];
    out << std::right << std::setw(3) << point.prev;
    out << std::right << std::setw(3) << point.next << " ";
    if (i == sector->bos_manager.pov) out << "P";
    if (i == sector->bos_manager.bos.First) out << "F";
    if (i == sector->bos_manager.bos.Last) out << "L";
    if (i == sector->bos_manager.bos.Head) out << "H";
    if (i == sector->bos_manager.bos.Tail) out << "T";
    out << "\n";
  }
  out << "H=" << sector->bos_manager.bos.Head;
  out << "\n";
  return out.str();
}

std::string sweepToASCII(Sector *sector, std::string direction) {
  std::stringstream out;
  LinkedList::LinkedListNode point;
  int end;
  int sweep = sector->bos_manager.pov;
  if (direction == "forward") {
    end = -1;
  } else if (direction == "backward") {
    end = -2;
  } else {
    LOGE << "Sweep direction neither forward nor backward";
    exit(1);
  }

  while (sweep != end) {
    point = sector->bos_manager.bos.LL[sweep];
    out << std::left << std::setw(3) << point.dem_id;
    out << std::left << std::setw(4) << sector->dem.elevations[point.dem_id];
    out << "\n";
    if (direction == "forward") {
      sweep = point.next;
    } else if (direction == "backward") {
      sweep = point.prev;
    }
  }
  return out.str();
}

