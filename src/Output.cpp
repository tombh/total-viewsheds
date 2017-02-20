#include <string>

#include <lodepng/lodepng.h>
#include <plog/Log.h>

#include "definitions.h"
#include "LinkedList.h"
#include "DEM.h"
#include "Output.h"
#include "Sector.h"

namespace TVS {

// DEMs can be very big so perhaps best to reference them rather than copy.
Output::Output(DEM &dem) : dem(dem) {}

void Output::tvsResults() {
  int col_step, row_step, point;
  FILE *fs;
  LOG_INFO << "Writing TVS results to file.";
  fs = fopen(TVS_RESULTS_FILE.c_str(), "ab");
  for (int x = 0; x < this->dem.width; x++) {
    row_step = this->dem.size + x;
    for (int y = 0; y < this->dem.height; y++) {
      col_step = ((y + 1) * this->dem.width);
      point = row_step - col_step;
      fwrite(&this->dem.cumulative_surface[point], 4, 1, fs);
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

// Convert computed TVS data into a PNG image. The resulting PNG has the same
// resolution as the DEM.
void Output::tvsToPNG() {
  this->palette = new color[FLAGS_size_of_tvs_png_palette];
  for (int i = 0; i < FLAGS_size_of_tvs_png_palette; i++) {
    this->palette[i] = this->getColorFromGradient(i);
  }
  std::vector<unsigned char> image = this->tvsArrayToPNGVect();
  lodepng::encode(TVS_PNG_FILE, image, this->dem.width, this->dem.height);
  delete [] this->palette;
}

std::vector<unsigned char> Output::tvsArrayToPNGVect() {
  std::vector<unsigned char> image;
  float max_value = this->dem.maxViewshedValue();
  float min_value = this->dem.minViewshedValue();
  float range = max_value - min_value;
  image.resize(this->dem.width * this->dem.height * 4);
  for (unsigned i = 0; i < this->dem.size; i++) {
    int v = (float)FLAGS_size_of_tvs_png_palette * ((this->dem.cumulative_surface[i] - min_value) / (range));
    if (v < 0) v = 0;
    if (v > FLAGS_size_of_tvs_png_palette - 1) v = FLAGS_size_of_tvs_png_palette - 1;
    image[4 * i + 0] = this->palette[v].R;
    image[4 * i + 1] = this->palette[v].G;
    image[4 * i + 2] = this->palette[v].B;
    image[4 * i + 3] = 255;  // alpha
  }
  return image;
}

// Converts final TVS to an ASCII grid.
// Eg for a simple 5x5 DEM;
// 0.215940  0.078242  0.098087  0.078242  0.089737
// 0.086336  0.141383  0.129477  0.141383  0.086336
// 0.105998  0.124606  0.161911  0.124606  0.105998
// 0.092023  0.119315  0.138933  0.119315  0.092023
// 0.096730  0.071249  0.084306  0.071249  0.096730
std::string Output::tvsToASCII() {
  std::string out;
  for (int point = 0; point < this->dem.size; point++) {
    out += std::to_string(this->dem.cumulative_surface[point]) + " ";
    if ((point % this->dem.width) == (this->dem.width - 1)) out += "\n";
  }
  out += "\n";
  return out;
}

Output::color Output::getColorFromGradient(int index) {
  float position = index;
  float size = FLAGS_size_of_tvs_png_palette;
  Output::color c = {255, 255, 255};  // white
  if (position < (0.25 * size)) {
    c.R = 0;
    c.G = 256 * (4 * position / size);
  } else if (position < (0.5 * size)) {
    c.R = 0;
    c.B = 256 * (1 + 4 * (0.25 * size - position) / size);
  } else if (position < (0.75 * size)) {
    c.R = 256 * (4 * (position - 0.5 * size) / size);
    c.B = 0;
  } else {
    c.G = 256 * (1 + 4 * (0.75 * size - position) / size);
    c.B = 0;
  }
  return (c);
}

std::string Output::viewshedToASCII(int viewer) {
  std::string viewshed_as_text;
  this->viewshed = new std::string[this->dem.size];
  this->viewer = viewer;
  this->fillViewshedWithDots();
  this->parseSectors();

  for (int position = 0; position < this->dem.size; position++) {
    viewshed_as_text += this->viewshed[position];
    if ((position % this->dem.width + 1) == this->dem.width)
      viewshed_as_text += "\n";
  }

  delete [] this->viewshed;
  return viewshed_as_text;
}

void Output::fillViewshedWithDots() {
  for (int point = 0; point < this->dem.size; point++) {
    this->viewshed[point] = ". ";
  }
}

void Output::parseSectors() {
  char path_ptr[100];
  for (int sector_angle = 0; sector_angle < FLAGS_total_sectors; sector_angle++) {
    Sector::ringSectorDataPath(path_ptr, sector_angle);
    this->sector_file = fopen(path_ptr, "rb");
    parseSectorPoints(this->sector_file);
    fclose(this->sector_file);
  }
}

// The format of a ring sector file is a simple array of 4 byte integer
// values. Sometimes they indicate the number of values that need to be
// parsed and sometimes they represent actual indexes for DEM points.
int Output::readNextValue() {
  int value;
  fread(&value, 4, 1, this->sector_file);
  return value;
}

// Remember that every sector computes every DEM point, just at a different
// angle.
// But we're just interested in one point (ie one viewshed) from each sector.
void Output::parseSectorPoints(FILE *sector_file) {
  int opening, closing;
  int no_of_ring_sectors;
  for (int point = 0; point < this->dem.size; point++) {
    // Backward and Forward facing portions of the sector
    for (int b_and_f = 0; b_and_f < 2; b_and_f++) {
      no_of_ring_sectors = this->getNumberOfRingSectors();
      for (int iRS = 0; iRS < no_of_ring_sectors; iRS++) {
        opening = readNextValue();
        closing = readNextValue();
        if (point == this->viewer && opening != closing) {
          this->viewshed[opening] = "+ ";
          this->viewshed[closing] = "- ";
        }
      }
    }
  }
}

// For every ring sector there will be an opening and a closing point,
// so the number of ring sectors will be half the number of items we
// need to parse.
int Output::getNumberOfRingSectors() {
  int value;
  int number_of_ring_sectors = 0;
  value = readNextValue();
  if (value != 0) number_of_ring_sectors = value / 2;
  return number_of_ring_sectors;
}

}  // namespace TVS
