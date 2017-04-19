#include "comparisons.hpp"

#include <cassert>

struct primitives {
  int int_member;
  unsigned uint_member;

  long long_member;
  unsigned long ulong_member;

  float float_member;
  double double_member;

  std::string string_member;
};

int main() {
  primitives a{1, 2, 3, 4, 5.6, 7.8, "hello world"};
  primitives b{1, 2, 3, 4, 5.6, 7.8, "hello world"};
  primitives c{9, 10, 11, 12, 13.14, 15.16, "foo"};

  assert(reflcompare::equal(a, b));
  assert(!reflcompare::equal(a, c));
}
