#include <string>
#include <vector>

#include "LinkedList.h"
#include "DEM.h"

namespace TVS {

class Output {
 public:
  struct color {
    unsigned char R;
    unsigned char G;
    unsigned char B;
  };

  DEM &dem;
  int viewer;
  FILE *sector_file;
  std::string *viewshed;

  Output(DEM &dem);
  void tvsResults();
  float readTVSFile();
  void tvsToPNG();
  std::string tvsToASCII();
  std::string viewshedToASCII(int);

 private:
  color *palette;

  std::vector<unsigned char> tvsArrayToPNGVect();
  color getColorFromGradient(int);
  void fillViewshedWithDots();
  void parseSectors();
  void parseSectorPoints(FILE *);
  int readNextValue();
  int getNumberOfRingSectors();
};
}
