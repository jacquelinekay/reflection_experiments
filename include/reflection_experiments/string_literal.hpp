#pragma once

#include <array>
#include <string>

#include <cstring>

namespace jk {
namespace string_literal {

// Heavily inspired by mpark and ubsan
// For some reason, auto and decltype(auto) crash Clang
#define CONSTANT(...) \
  struct { \
    static constexpr jk::string_literal::string_literal value() { return __VA_ARGS__; } \
  } \

struct string_literal {
  constexpr string_literal(const char* v, unsigned x) : value(v), s(x) { }
  constexpr auto size() const { return s; };
  constexpr auto data() const { return value; };
  constexpr auto char_at(unsigned i) const { return value[i]; };
private:
  const char* value;
  const unsigned s;
};

constexpr static unsigned length(const char* str) {
  return *str ? 1 + length(str + 1) : 0;
}


template<auto V>
struct string_constant {
  static constexpr auto value() {
    return string_literal(V, length(V));
  };
};

// what if this just was the string literal struct
#define STRING_TYPE_DECL(StructName, ...) \
  struct StructName { \
    static constexpr jk::string_literal::string_literal value() { \
      return jk::string_literal::string_literal(__VA_ARGS__, sizeof(__VA_ARGS__) - 1); \
    } \
  };

#define STRING_LITERAL(value) \
  []() { \
    using Str = CONSTANT(jk::string_literal::string_literal{value, sizeof(value) - 1}); \
    return Str{}; \
  }() \

#define STRING_LITERAL_VALUE(value) \
  jk::string_literal::string_literal{value, sizeof(value) - 1}

template<typename Str>
struct compare_helper {
  template<size_t... I>
  constexpr static bool apply(const char* v, std::index_sequence<I...>) {
    return ((Str::value().char_at(I) == v[I]) && ...);
  }
};

template<typename Str>
constexpr static bool empty(const Str&) {
  return Str::value().size() == 0;
}

template<typename Str>
constexpr static bool equal(const Str&, const Str& b) {
  return Str::value().data() == b.value().data();
}

template<typename Str>
constexpr static bool equal(const Str&, const char* b) {
  if (length(b) != Str::value().size()) {
    return false;
  } else {
    return compare_helper<Str>::apply(b, std::make_index_sequence<Str::value().size()>{});
  }
}

STRING_TYPE_DECL(empty_string_t, "")

}  // namespace string_literal
}  // namespace jk
