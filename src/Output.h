#include <string>
#include <vector>

#include "DEM.h"
#include "definitions.h"

namespace TVS {

class Output {
 public:
  struct color {
    unsigned char R;
    unsigned char G;
    unsigned char B;
  };

  DEM &dem;
  std::string viewshed[DEM_SIZE];
  int viewer;
  FILE *sector_file;

  Output(DEM &dem);
  void tvsResults();
  float readTVSFile();
  void tvsToPNG();
  std::string tvsToASCII();
  std::string viewshedToASCII(int);

 private:
  int size_of_palette;
  color palette[SIZE_OF_TVS_PNG_PALETTE];

  std::vector<unsigned char> tvsArrayToPNGVect();
  color getColorFromGradient(int);
  void fillViewshedWithDots();
  void parseSectors();
  void parseSectorPoints(FILE *);
  int readNextValue();
  int getNumberOfRingSectors();
};
}
