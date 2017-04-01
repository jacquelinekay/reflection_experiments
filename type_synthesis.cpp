/* Type synthesis with reflexpr proof of concept
 *
 * This example assumes default-constructible parameters.
 * */

#include <iostream>

#include "type_synthesis.hpp"

struct old_type {
  int foo;
  float baz;
};

int main() {
  using namespace type_synthesis;
  using new_type =
      synthesize_type<old_type, add_member<std::string, decltype("bar"_s)>,
                      remove_member<decltype("baz"_s)>>::result;

  // Does not compile: cannot remove qux because it is not a member of the original type.
  // using new_type = synthesize_type<old_type, remove_member<decltype("qux"_s)>>::result;

  // Does not compile: foo is already a member of the original type, we can't add it.
  // using new_type = synthesize_type<old_type, add_member<char, decltype("foo"_s)>>::result;

  new_type x;

  auto foo_v = access(x, "foo"_s);
  static_assert(std::is_same<std::decay_t<decltype(foo_v)>, int>{});

  auto bar_v = access(x, "bar"_s);
  static_assert(std::is_same<std::decay_t<decltype(bar_v)>, std::string>{});

  // Does not compile: baz was removed from the synthesized type
  // access(x, "baz"_s);

  // Does not compile: qux is not a member of the synthesized type
  // access(x, "qux"_s);
}
