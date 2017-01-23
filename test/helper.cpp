#include <stdio.h>
#include <sstream>

#include "helper.h"
#include "../src/definitions.h"

// Create a fake DEM for testing purposes
void createMockDEM(const int dem[]) {
	FILE *f;
	f = fopen(INPUT_DEM_FILE, "wb");

	if(f == NULL) {
		LOG_ERROR << "Error opening mock DEM, tests should be run from the base project path.";
	} else {
		for(int i = 0; i < 25; i++)
		{
			fwrite(&dem[i], 2, 1, f);
		}
	}
  fclose(f);
}

void emptyDirectories() {
  std::stringstream cmd;

  cmd << "rm -rf " << SECTOR_DIR << "/*.bin";
  system(cmd.str().c_str());

  cmd << "rm -rf " << RING_SECTOR_DIR << "/*.bin";
  system(cmd.str().c_str());
}

void setup() {
  LOG_INFO << "Started tests setup.";
  emptyDirectories();
}

void tearDown() {
  emptyDirectories();
  LOG_INFO << "Finished tests tear down.";
}

