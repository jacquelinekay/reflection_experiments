/* Idea: implement language-level "deep structured bindings" (destructuring of arbitrary nested POD types)
 * */

struct foo {
  std::string bar;
  int baz;
};

struct foo_result {
  int retCode;
  foo result;
};

foo_result foo_factory() {
  foo_result result;
  result.retCode = 1;
  result.foo.bar = "hello";
  result.foo.baz = 42;
  return
}

int main() {
  DECOMPOSE(int x, foo{std::string y, int z}) = foo_factory();
  std::cout << x << "\n";
  std::cout << y << "\n";
  std::cout << z << "\n";
}
