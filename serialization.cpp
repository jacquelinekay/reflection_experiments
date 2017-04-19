/* it would be great generate comprehensive tests, maybe piggyback on an 
 * */
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
};

template<auto N>
struct arrays_of_primitives {
  std::array<int, N> int_member;
  std::array<unsigned, N> uint_member;

  std::array<long, N> long_member;
  std::array<unsigned long, N> ulong_member;

  std::array<float, N> float_member;
  std::array<double, N> double_member;

  std::array<std::string, N> string_member;
};

template<auto N>
struct vectors_of_primitives {
  std::vector<int> int_member;
  std::vector<unsigned> uint_member;

  std::vector<long> long_member;
  std::vector<unsigned long> ulong_member;

  std::vector<float> float_member;
  std::vector<double> double_member;

  std::vector<std::string> string_member;
};

struct nested {
  primitives p;
};


int main(int argc, char** argv) {
  {
    primitives test{1, 2, 3, 4, 5.6, 7.8, "hello world"};
    std::string dst = "";
    auto result = reflser::serialize(test, dst);
    std::cout << dst << "\n";

    primitives deserialized;
    std::string_view dst_view = dst;
    // values appear to be totally screwed here!
    if (auto result = reflser::deserialize(dst_view, deserialized); result != reflser::deserialize_result::success) {
      std::cout << reflser::deserialize_result_message(result) << "\n";
    }
    dst = "";

    auto result2 = reflser::serialize(deserialized, dst);
    std::cout << dst << "\n";
  }
}
