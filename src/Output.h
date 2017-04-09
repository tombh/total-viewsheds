#include <string>
#include <vector>

#include "DEM.h"

namespace TVS {

class Output {
 public:
  DEM &dem;
  int viewer;
  FILE *sector_file;
  std::string *viewshed;

  Output(DEM &dem);
  void tvsResults();
  float readTVSFile();
  std::string tvsToASCII();
  std::string viewshedToASCII(int);
  std::vector<std::vector<int>> parseSingleSector(int, int);

 private:
  void fillViewshedWithDots();
  void parseSectors();
  std::vector<std::vector<int>> parseSectorPoints();
  int readNextValue();
  void populateViewshed(int, int);
};
}
