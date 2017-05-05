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
  primitives primitives_member;
  vectors_of_primitives vector_member;
};

template<typename T>
int test(const T& reference, const std::string& test_string) {
  std::string dst = "";
  if (auto result = reflser::serialize(reference, dst);
      result != reflser::serialize_result::success) {
    std::cout << reflser::serialize_result_message(result) << "\n";
    return 255;
  }
  std::cout << dst << "\n";
  std::cout << test_string << "\n";
  assert(dst == test_string);

  T deserialized;
  std::string_view dst_view = dst;
  if (auto result = reflser::deserialize(dst_view, deserialized);
      result != reflser::deserialize_result::success) {
    std::cout << reflser::deserialize_result_message(result) << "\n";
    return 255;
  }

  dst = "";

  if (auto result = reflser::serialize(deserialized, dst);
      result != reflser::serialize_result::success) {
    std::cout << reflser::serialize_result_message(result) << "\n";
    return 255;
  }
  std::cout << dst << "\n";
  std::cout << test_string << "\n";
  assert(dst == test_string);

  assert(reflcompare::equal(reference, deserialized));
  return 0;
};

int main(int argc, char** argv) {
  const primitives primitives_test{1, 2, 3, 4, 5.6, 7.8, "hello world", true};
  const arrays_of_primitives<2> aop_test{
    {1, 2}, {3, 4}, {5, 6}, {7, 8}, {9.1, 10.2}, {11.3, 12.4}, {"hello", "world"}};

  const int vop_size = 3;
  vectors_of_primitives vop_test;
  for (int i = 0; i < vop_size; ++i) {
    vop_test.int_member.push_back(i);
    vop_test.uint_member.push_back(i + 1);

    vop_test.long_member.push_back(i + 2);
    vop_test.ulong_member.push_back(i + 3);

    vop_test.float_member.push_back(i + 4.1);
    vop_test.double_member.push_back(i + 5.1);
  }
  vop_test.string_member = {"hello", "world", "!"};

  const nested nested_test{primitives_test, vop_test};

  // Primitives
  const std::string primitives_test_string = "{ \"int_member\" : 1, \"uint_member\" : 2, \"long_member\" : 3, \"ulong_member\" : 4, \"float_member\" : 5.600000, \"double_member\" : 7.800000, \"string_member\" : \"hello world\", \"bool_member\" : true }";
  {
    if (int retcode = test(primitives_test, primitives_test_string); retcode != 0) {
      return retcode;
    }
  }

  // Array of primitives
  {
    const std::string test_string = "{ \"int_member\" : [ 1, 2 ], \"uint_member\" : [ 3, 4 ], \"long_member\" : [ 5, 6 ], \"ulong_member\" : [ 7, 8 ], \"float_member\" : [ 9.100000, 10.200000 ], \"double_member\" : [ 11.300000, 12.400000 ], \"string_member\" : [ \"hello\", \"world\" ] }";
    if (int retcode = test(aop_test, test_string); retcode != 0) {
      return retcode;
    }
  }

  const std::string vop_test_string = "{ \"int_member\" : [ 0, 1, 2 ], \"uint_member\" : [ 1, 2, 3 ], \"long_member\" : [ 2, 3, 4 ], \"ulong_member\" : [ 3, 4, 5 ], \"float_member\" : [ 4.100000, 5.100000, 6.100000 ], \"double_member\" : [ 5.100000, 6.100000, 7.100000 ], \"string_member\" : [ \"hello\", \"world\", \"!\" ] }";
  // Vector of primitives
  {
    if (int retcode = test(vop_test, vop_test_string); retcode != 0) {
      return retcode;
    }
  }

  {
    const std::string test_string = "{ \"primitives_member\" : " + primitives_test_string
      + ", \"vector_member\" : " + vop_test_string + " }";
    if (int retcode = test(nested_test, test_string); retcode != 0) {
      return retcode;
    }
  }

  return 0;
}
