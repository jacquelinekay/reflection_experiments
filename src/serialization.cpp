#include "comparisons.hpp"
#include "reflser.hpp"

#include <array>
#include <iostream>
#include <vector>

struct primitives {
  int int_member;
  unsigned uint_member;

  long long_member;
  unsigned long ulong_member;

  float float_member;
  double double_member;

  std::string string_member;

  bool bool_member;
};

int main(int argc, char** argv) {
  primitives primitives_test{1, 2, 3, 4, 5.6, 7.8, "hello world", true};
  {
    std::string dst = "";
    auto result = reflser::serialize(primitives_test, dst);
    std::cout << dst << "\n";

    primitives deserialized;
    std::string_view dst_view = dst;
    if (auto result = reflser::deserialize(dst_view, deserialized); result != reflser::deserialize_result::success) {
      std::cout << reflser::deserialize_result_message(result) << "\n";
      return 255;
    }

    dst = "";

    reflser::serialize(deserialized, dst);
    std::cout << dst << "\n";

    assert(reflcompare::equal(primitives_test, deserialized));
  }
}
