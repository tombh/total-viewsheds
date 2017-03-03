#ifndef TVS_FIXTURES_H
#define TVS_FIXTURES_H

namespace TVS {
namespace fixtures {

const int mountainDEM[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 0,
  0, 1, 3, 3, 3, 3, 3, 1, 0,
  0, 1, 3, 6, 6, 6, 3, 1, 0,
  0, 1, 3, 6, 9, 6, 3, 1, 0,
  0, 1, 3, 6, 6, 6, 3, 1, 0,
  0, 1, 3, 3, 3, 3, 3, 1, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0
};

const int doublePeakDEM[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 0,
  0, 1, 3, 3, 3, 3, 3, 3, 4,
  0, 1, 3, 4, 4, 4, 4, 4, 3,
  0, 1, 3, 4, 6, 4, 4, 4, 3,
  0, 1, 3, 4, 4, 4, 5, 5, 3,
  0, 1, 3, 4, 4, 5, 9, 5, 3,
  0, 1, 1, 4, 4, 5, 5, 5, 3,
  0, 0, 4, 1, 3, 3, 3, 3, 3
};

}
}

#endif
