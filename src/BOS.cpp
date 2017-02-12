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
      band_size(FLAGS_dem_width),
      half_band_size((band_size - 1) / 2),
      // The data structure maintaining the band of sight
      bos(LinkedList(band_size)) {}

BOS::~BOS() {
  // TODO: it should be LinkedList's responsibility to delete this.
  // But using LinkedList's destructor causes LL to be deleted prematurely.
  delete[] this->bos.LL;
}

// Set the band of sight up for the very first time, namely after a sector angle
// change.
void BOS::setup(FILE *precomputed_data_file) {
  this->precomputed_data_file = precomputed_data_file;
  this->pov = 0;
  this->bos.Clear();
  int first_ordered_node = this->dem.nodes_sector_ordered[0];
  this->bos.FirstNode(this->dem.nodes[first_ordered_node]);
}

// Set the index of the currently analysed point to be relative to the band
// of site size.
void BOS::setPointOfView() {
  this->pov = this->current_point % this->band_size;
}

void BOS::adjustToNextPoint(int current_point) {
  this->current_point = current_point;
  this->setPointOfView();
  // Is the band of sight starting to be populated?
  bool starting = (this->current_point < this->half_band_size);
  // Is the band of sight coming to an ending, therefore emptying?
  bool ending =
      (this->current_point >= (this->dem.size - this->half_band_size - 1));


  this->NEW_position = -1;
  this->NEW_position_trans = -1;
  LinkedList::node tmp = this->newnode_trans;
  if (!ending) {
    this->newnode =
        this->dem.nodes[this->dem.nodes_sector_ordered
                            [starting ? 2 * this->current_point + 1
                                      : this->half_band_size +
                                            this->current_point + 1]];
  }
  if (starting) {
    this->newnode_trans =
        this->dem
            .nodes[this->dem.nodes_sector_ordered[2 * this->current_point + 2]];
  }

  if (this->dem.is_computing) {
    if (!ending) {
      fread(&this->NEW_position, 4, 1, this->precomputed_data_file);
      this->insertNode(this->newnode, this->NEW_position, !starting);
    }
    if (starting) {
      fread(&this->NEW_position_trans, 4, 1, this->precomputed_data_file);
      this->insertNode(this->newnode_trans, this->NEW_position_trans, false);
    }
  } else {
    if (!starting && !ending) {
      this->newnode =
          this->dem.nodes[this->dem.nodes_sector_ordered
                              [starting ? 2 * this->current_point + 1
                                        : this->half_band_size +
                                              this->current_point + 1]];
      this->NEW_position = -1;
      this->calculateNewnodePosition(true);
    }
    if (starting) {
      this->newnode =
          this->dem
              .nodes[this->dem.nodes_sector_ordered[2 * this->current_point + 1]];
      this->NEW_position = -1;
      this->calculateNewnodePosition(false);
      this->newnode =
          this->dem
              .nodes[this->dem.nodes_sector_ordered[2 * this->current_point + 2]];
      this->calculateNewnodePosition(false);
    }
  }
  if (ending) this->bos.Remove_two();
}

void BOS::calculateNewnodePosition(bool remove) {
  int sweep = 0;
  if (newnode.sight_ordered_index < this->bos.LL[this->bos.First].Value.sight_ordered_index) {
    this->NEW_position = -2;  // Before first
    this->bos.AddFirst(newnode, remove);
    fwrite(&this->NEW_position, 4, 1, this->precomputed_data_file);
  } else if (newnode.sight_ordered_index > this->bos.LL[this->bos.Last].Value.sight_ordered_index) {
    this->NEW_position = -1;
    this->bos.AddLast(newnode, remove);
    fwrite(&this->NEW_position, 4, 1, this->precomputed_data_file);
  } else {
    sweep = this->bos.LL[this->bos.First].next;
    bool go_on = true;
    int sanity = 0;

    do {
      if (newnode.sight_ordered_index < this->bos.LL[sweep].Value.sight_ordered_index) {
        NEW_position = this->bos.LL[sweep].prev;
        this->bos.Add(newnode, NEW_position, remove);
        fwrite(&NEW_position, 4, 1, this->precomputed_data_file);
        sweep = this->bos.LL[NEW_position].next;
        go_on = false;
      } else {
        sweep = this->bos.LL[sweep].next;
        sanity++;
        if (sanity > this->dem.size) {
          LOGE << "newnode.sight_ordered_index: " << newnode.sight_ordered_index
               << " is bigger than anything in band of sight.";
          exit(1);
        }
      }
    } while (go_on);
  }
}

void BOS::insertNode(LinkedList::node newnode, int position, bool remove) {
  if (position > -1) {
    this->bos.Add(newnode, position, remove);
  } else {
    if (position == -1) {
      this->bos.AddLast(newnode, remove);
    } else {
      this->bos.AddFirst(newnode, remove);
    }
  }
}

}  // namespace TVS
