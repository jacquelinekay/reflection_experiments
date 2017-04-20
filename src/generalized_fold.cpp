#include "generalized_fold.hpp"

#include <iostream>
#include <string>

int main() {
  using namespace jk;
  auto cat = [](auto x, auto y){ return "f(" + x + ", " + y + ")"; };
  std::string a = "a";
  std::string b = "b";
  std::string c = "c";
  // expect f(f(a, b), c)
  std::cout << fold_left(cat, a, b, c) << "\n";
  // expect f(f(a, b), c)
  std::cout << fold(cat, a, b, c) << "\n";
  // expect f(a, f(b, c))
  std::cout << fold_right(cat, a, b, c) << "\n";
}
