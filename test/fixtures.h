#ifndef TVS_FIXTURES_H
#define TVS_FIXTURES_H

namespace TVS {
namespace fixtures {

const int mountainDEM[] = {
  0, 0, 0, 0, 0,
  0, 5, 5, 5, 0,
  0, 5, 9, 5, 0,
  0, 5, 5, 5, 0,
  0, 0, 0, 0, 0
};

const int doublePeakDEM[] = {
  0, 3, 3, 0, 0,
  3, 3, 0, 0, 7,
  3, 0, 0, 7, 7,
  0, 0, 7, 7, 9,
  0, 7, 7, 9, 9
};

}
}

#endif
