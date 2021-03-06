#include <string>

#include <plog/Log.h>

#include "DEM.h"
#include "Output.h"
#include "Viewsheds.h"
#include "definitions.h"

namespace TVS {

Output::Output(DEM &dem) : dem(dem), viewshed(new std::string[dem.size]) {};

void Output::tvsResults() {
  int col_step, row_step, point;
  FILE *fs;
  LOG_INFO << "Writing TVS results to file.";
  fs = fopen(TVS_RESULTS_FILE.c_str(), "ab");
  for (int x = 0; x < this->dem.tvs_width; x++) {
    row_step = this->dem.computable_points_count + x;
    for (int y = 0; y < this->dem.tvs_width; y++) {
      col_step = ((y + 1) * this->dem.tvs_width);
      point = row_step - col_step;
      fwrite(&this->dem.tvs_complete[point], 4, 1, fs);
    }
  }
  fclose(fs);
}

float Output::readTVSFile() {
  float tvs_data[this->dem.size];
  float tvs_value;
  FILE *tvs_file = fopen(TVS_RESULTS_FILE.c_str(), "rb");
  for (int point = 0; point < this->dem.size; point++) {
    fread(&tvs_value, 4, 1, tvs_file);
    tvs_data[point] = tvs_value;
  }
  fclose(tvs_file);
  return *tvs_data;
}

// Converts final TVS to an ASCII grid.
// Eg; for a simple 9x9 DEM:
// 0.215940  0.078242  0.098087
// 0.086336  0.141383  0.129477
// 0.105998  0.124606  0.161911
std::string Output::tvsToASCII() {
  std::string out;
  for (int point = 0; point < this->dem.computable_points_count; point++) {
    out += std::to_string(this->dem.tvs_complete[point]) + " ";
    if ((point % this->dem.tvs_width) == (this->dem.tvs_width - 1)) out += "\n";
  }
  out += "\n";
  return out;
}

std::string Output::viewshedToASCII(int viewer) {
  std::string viewshed_as_text = "";
  this->viewer = viewer;
  this->fillViewshedWithDots();
  this->parseSectors();

  for (int position = 0; position < this->dem.size; position++) {
    if (position == viewer) {
      viewshed_as_text += "o ";
    } else {
      viewshed_as_text += this->viewshed[position];
    }
    if ((position % this->dem.width + 1) == this->dem.width) {
      viewshed_as_text += "\n";
    }
  }

  delete[] this->viewshed;
  return viewshed_as_text;
}

void Output::fillViewshedWithDots() {
  for (int point = 0; point < this->dem.size; point++) {
    this->viewshed[point] = ". ";
  }
}

void Output::parseSectors() {
  for (int sector_angle = 0; sector_angle < FLAGS_total_sectors; sector_angle++) {
    this->parseSingleSector(sector_angle, this->viewer);
  }
}

std::vector<std::vector<int>> Output::parseSingleSector(int angle, int viewer) {
  char path_ptr[100];
  this->viewer = viewer;
  Viewsheds::ringDataPath(path_ptr, angle);
  this->sector_file = fopen(path_ptr, "rb");
  std::vector<std::vector<int>> single_band = parseSectorPoints();
  fclose(this->sector_file);
  return single_band;
}

// The format of a ring sector file is a simple array of 4 byte integer
// values. Sometimes they indicate the number of values that need to be
// parsed and sometimes they represent actual indexes for DEM points.
int Output::readNextValue() {
  int value;
  fread(&value, 4, 1, this->sector_file);
  return value;
}

std::vector<std::vector<int>> Output::parseSectorPoints() {
  int opening, closing, pov, no_of_ring_values;
  std::vector<int> front_band, back_band;
  std::vector<std::vector<int>> single_band;

  for (int point = 0; point < this->dem.computable_points_count * 2; point++) {
    no_of_ring_values = readNextValue() / 2;

    // Assume that every DEM point has an opening at the PoV
    pov = readNextValue();

    if (pov == this->viewer) {
      if (point < this->dem.computable_points_count) {
        front_band.push_back(no_of_ring_values);
      } else {
        back_band.push_back(no_of_ring_values);
      }
    }

    for (int iRS = 0; iRS < no_of_ring_values; iRS++) {
      if (iRS == 0) {
        opening = pov;
      } else {
        opening = readNextValue();
      }
      closing = readNextValue();

      if (pov == this->viewer) {
        if (point < this->dem.computable_points_count) {
          front_band.push_back(opening);
          front_band.push_back(closing);
        } else {
          back_band.push_back(opening);
          back_band.push_back(closing);
        }
        this->populateViewshed(opening, closing);
      }
    }
  }

  single_band.push_back(front_band);
  single_band.push_back(back_band);
  return single_band;
}

void Output::populateViewshed(int opening, int closing) {
  if (this->viewshed[opening] == ". ") {
    this->viewshed[opening] = "+ ";
  } else {
    this->viewshed[opening] = "± ";
  }

  if (this->viewshed[closing] == ". ") {
    this->viewshed[closing] = "- ";
  } else {
    this->viewshed[closing] = "± ";
  }

  if (opening == closing) {
    this->viewshed[opening] = "± ";
  }
}

}  // namespace TVS
