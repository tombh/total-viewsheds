#include <plog/Log.h>

#include "BOS.h"
#include "DEM.h"
#include "definitions.h"

namespace TVS {

/**
 * The Band of Sight (BoS) is essentially the line of sight. In other
 * words: which DEM points fall along a line 'as the crow flies'.
 *
 * The conception of the BoS here is different to traditional 'V'-shaped
 * methods. Here we use a parallel band. This is fundamental to the
 * optimization improvements found in this whole approach.
**/

BOS::BOS(DEM &dem)
    : dem(dem),
      // Size of the Band of Sight
      band_size(ensureBandSizeIsOdd(FLAGS_dem_width)),
      bos(LinkedList(band_size)) {}

// Currently, the adjustments to new BoS nodes requires the Bos itself to have
// an odd size. Roughly speaking, think of it like this:
//
//   BoS = half_band_size + PoV + half_band_size
//
// It's easier to just override band_size than rewrite the bulk of the code
// to support even-sized band sizes.
int BOS::ensureBandSizeIsOdd(int requested_band_size) {
  if (requested_band_size % 2 == 0) {
    requested_band_size++;
    LOGI << "Raising `band_size` from " << requested_band_size - 1 << " to "
         << requested_band_size;
  }
  return requested_band_size;
}

void BOS::openPreComputedDataFile() {
  const char *mode;
  char path[100];
  this->preComputedDataPath(path, this->sector_angle);
  if (this->dem.is_precomputing) {
    mode = "wb";
  } else {
    mode = "rb";
  }
  this->precomputed_data_file = fopen(path, mode);
  if (this->precomputed_data_file == NULL) {
    LOG_ERROR << "Couldn't open " << path
              << " for sector_angle: " << this->sector_angle;
    throw "Couldn't open file.";
  }
}

void BOS::preComputedDataPath(char *path_ptr, int sector_angle) {
  sprintf(path_ptr, "%s/%d.bin", SECTOR_DIR.c_str(), sector_angle);
}

// Set the band of sight up for the very first time, namely after a sector angle
// change.
void BOS::setup(int sector_angle) {
  this->positions = new int[this->dem.size];
  this->sector_angle = sector_angle;
  this->half_band_size = (this->band_size - 1) / 2;
  this->current_point = 0;
  this->openPreComputedDataFile();
  if(this->dem.is_computing) {
    fread(this->positions, 4, this->dem.size, this->precomputed_data_file);
  }
  this->pov = 0;
  this->bos.Clear();
  int first_ordered_node = this->dem.nodes_sector_ordered[0];
  this->bos.FirstNode(this->dem.nodes[first_ordered_node]);
}

void BOS::writeAndClose() {
  if (this->dem.is_precomputing && this->current_point == this->dem.size - 1) {
    fwrite(this->positions, 4, this->dem.size, this->precomputed_data_file);
  }
  fclose(this->precomputed_data_file);
  delete[] this->positions;
}

// Set the index of the currently analysed point to be relative to the band
// of site size.
void BOS::setPointOfView() {
  this->pov = this->current_point % this->band_size;
}

// TODO: support even band sizes.
// Anchoring to the PoV requires managing the edges of the BoS by carefully
// using the half_band_size. This has the disadvantage of only supporting
// odd band sizes.
void BOS::adjustToNextPoint(int current_point) {
  this->current_point = current_point;
  this->setPointOfView();

  // Is the band of sight starting to be populated?
  bool starting = this->current_point < this->half_band_size;

  // Is the band of sight coming to an ending, therefore emptying?
  int end_section = this->dem.size - this->half_band_size - 1;
  bool ending = this->current_point >= end_section;

  int node_id, leading_node;
  int doubled = 2 * this->current_point;

  if (starting) {
    this->remove = false;

    node_id = this->dem.nodes_sector_ordered[doubled + 1];
    this->newnode = this->dem.nodes[node_id];
    this->insertNode();

    node_id = this->dem.nodes_sector_ordered[doubled + 2];
    this->newnode = this->dem.nodes[node_id];
    this->insertNode();
  }

  if (!starting && !ending) {
    this->remove = true;
    leading_node = this->current_point + this->half_band_size + 1;
    node_id = this->dem.nodes_sector_ordered[leading_node];
    this->newnode = this->dem.nodes[node_id];
    this->insertNode();
  }

  if (ending) {
    this->bos.Remove_one();
    this->bos.Remove_one();
  }
}

// Where does the new node fit in the current BoS?
int BOS::calculateNewPosition() {
  int position;
  int sweep;
  int sanity = 0;
  int current_sight_index = this->newnode.sight_ordered_index;
  int first_sight_index =
      this->bos.LL[this->bos.First].Value.sight_ordered_index;
  int last_sight_index = this->bos.LL[this->bos.Last].Value.sight_ordered_index;

  if (current_sight_index < first_sight_index) {
    position = -2;
  } else if (current_sight_index > last_sight_index) {
    position = -1;
  } else {
    sweep = this->bos.LL[this->bos.First].next;
    while (current_sight_index >=
           this->bos.LL[sweep].Value.sight_ordered_index) {
      sweep = this->bos.LL[sweep].next;
      sanity++;
      if (sanity > this->dem.size) {
        LOGE << "newnode.sight_ordered_index: " << newnode.sight_ordered_index
             << " is bigger than anything in band of sight.";
        throw;
      }
    }
    position = this->bos.LL[sweep].prev;
  }
  return position;
}

int BOS::getNewPosition() {
  int position;
  if (this->dem.is_computing) {
    position = this->positions[this->newnode.idx];
  }
  if (this->dem.is_precomputing) {
    position = this->calculateNewPosition();
    this->positions[this->newnode.idx] = position;
  }
  return position;
}

void BOS::insertNode() {
  int position = this->getNewPosition();
  if (position > -1) {
    this->bos.Add(this->newnode, position, this->remove);
  } else {
    if (position == -1) {
      this->bos.AddLast(this->newnode, this->remove);
    }
    if (position == -2) {
      this->bos.AddFirst(this->newnode, this->remove);
    }
  }
}

}  // namespace TVS
