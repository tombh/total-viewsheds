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
#include "../src/Sector.h"
#include "../src/Axes.h"
#include "helper.h"

// Create a fake DEM for testing purposes
void createMockDEM(const int dem[]) {
  int row_step, col_step, i;
  FILE* f;
  f = fopen(INPUT_DEM_FILE.c_str(), "wb");
  // Skip the header as if it were there
  fseek(f, 256, SEEK_SET);

  if (f == NULL) {
    throw "Error opening mock DEM, tests should be run from the base "
          "project path.";
  } else {
    // Convert to .bt format
    for (int x = 0; x < 5; x++) {
      row_step = 25 + x;
      for (int y = 0; y < 5; y++) {
        col_step = (y + 1) * 5;
        i = row_step - col_step;
        fwrite(&dem[i], 2, 1, f);
      }
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

void setFLAGDefaults() {
  FLAGS_dem_width = 5;
  FLAGS_dem_height = 5;
  FLAGS_is_store_ring_sectors = false;
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
  setFLAGDefaults();
  emptyDirectories();
  helper::createDirectories();
}

void tearDown() {
  emptyDirectories();
}

void computeDEMFor(DEM *dem, int angle) {
  Axes axes = Axes(*dem);
  axes.adjust(angle);
}

std::string sectorOrderedDEMPointsToASCII(DEM *dem) {
  std::stringstream out;
  int inverted[dem->size];
  for (int point = 0; point < dem->size; point++) {
    inverted[dem->nodes_sector_ordered[point]] = point;
  }
  for (int point = 0; point < dem->size; point++) {
    out << std::right << std::setw(3) << std::setfill(' ')
      << std::to_string(inverted[point]);
    if ((point % dem->width) == (dem->width - 1)) out << "\n";
  }
  out << "\n";
  return out.str();
}

std::string sightOrderedDEMPointsToASCII(DEM *dem) {
  std::stringstream out;
  for (int point = 0; point < dem->size; point++) {
    out << std::right << std::setw(3) << std::setfill(' ')
      << std::to_string(dem->nodes[point].sight_ordered_index);
    if ((point % dem->width) == (dem->width - 1)) out << "\n";
  }
  out << "\n";
  return out.str();
}

std::string nodeDistancesToASCII(DEM *dem) {
  std::stringstream out;
  for (int point = 0; point < dem->size; point++) {
    out << std::to_string(dem->nodes[point].d) << " ";
    if ((point % dem->width) == (dem->width - 1)) out << "\n";
  }
  out << "\n";
  return out.str();
}

void computeBOSFor(Sector *sector, int angle, int point, std::string ordering) {
  sector->dem.setNodeIDs();
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

void bosLoopToDEMPoint(Sector *sector, int dem_point) {
  for (int i = 0; i < sector->dem.size; i++) {
    sector->bos_manager.adjustToNextPoint(i);
    int idx = sector->bos_manager.bos.LL[sector->bos_manager.pov].Value.idx;
    if(idx == dem_point) break;
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
  LinkedList::LinkedListNode node;
  for(int i = 0; i < sector->bos_manager.bos.Count; i++){
    node = sector->bos_manager.bos.LL[i];
    out << std::left << std::setw(2) << i;
    out << std::left << std::setw(3) << node.Value.idx;
    out << std::left << std::setw(3) << node.Value.sight_ordered_index;
    out << std::left << std::setw(8) << node.Value.d;
    out << std::right << std::setw(3) << node.prev;
    out << std::right << std::setw(3) << node.next << " ";
    if(i == sector->bos_manager.pov) out << "P";
    if(i == sector->bos_manager.bos.First) out << "F";
    if(i == sector->bos_manager.bos.Last) out << "L";
    if(i == sector->bos_manager.bos.Head) out << "H";
    if(i == sector->bos_manager.bos.Tail) out << "T";
    out << "\n";
  }
  out << "H=" << sector->bos_manager.bos.Head;
  out << "\n";
  return out.str();
}

std::string sweepToASCII(Sector *sector, std::string direction){
  std::stringstream out;
  LinkedList::LinkedListNode node;
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

  while(sweep != end){
    node = sector->bos_manager.bos.LL[sweep];
    out << std::left << std::setw(3) << node.Value.idx;
    out << std::left << std::setw(4) << node.Value.h;
    out << "\n";
    if (direction == "forward") {
      sweep = node.next;
    } else if (direction == "backward") {
      sweep = node.prev;
    }
  }
  return out.str();
}

void computeSweepFor(Compute *compute, std::string direction, int angle, int point) {
  FLAGS_is_store_ring_sectors = true;
  compute->dem.setHeights();
  computeBOSFor(&compute->sector, angle, point, "dem-indexed");
  compute->sector.sweepInit();
  if (direction == "forward") {
    compute->sector.sweepForward();
  } else if (direction == "backward") {
    compute->sector.sweepBackward();
  } else {
    LOGE << "Sweep direction neither forward nor backward";
    exit(1);
  }
}

