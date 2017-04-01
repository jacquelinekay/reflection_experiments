#pragma once

#include <string>

namespace jk {
namespace string_literal {

template <char...> struct string_literal {};

}  // namespace string_literal
}  // namespacejk 

template <typename charT, charT... Pack>
constexpr jk::string_literal::string_literal<Pack...> operator""_s() {
  return {};
}

namespace jk {
namespace string_literal {

template<typename T>
struct empty : std::bool_constant<std::is_same<T, string_literal<>>{}> { };

template<typename String>
struct string_length;

template<char... Pack>
struct string_length<string_literal<Pack...>> : std::integral_constant<size_t, sizeof...(Pack)> {};

// Convert from string_literal to const char
template<typename charT, charT ...Pack>
constexpr std::array<charT, sizeof...(Pack)> pack_to_literal() {
  return {Pack...};
}

template<typename charT, charT ...Pack>
std::string pack_to_string() {
  return (std::string() + ... + Pack);
}

template<typename T, typename charT = char>
struct literal_type_to_string;

template<typename charT, charT ...Pack>
struct literal_type_to_string<string_literal<Pack...>, charT> {
  static std::string convert() {
    return pack_to_string<charT, Pack...>();
  }
};

template <char... Pack>
using char_sequence = std::integer_sequence<char, Pack...>;

// adapted from http://ldionne.com/2015/11/29/efficient-parameter-pack-indexing/
template <std::size_t I, char T> struct indexed {
  constexpr static char value = T;
};

template <typename Is, char... Ts> struct indexer;

template <std::size_t... Is, char... Ts>
struct indexer<std::index_sequence<Is...>, Ts...> : indexed<Is, Ts>... {};

template <std::size_t I, char T> constexpr static char select(indexed<I, T>) {
  return indexed<I, T>::value;
}

template <std::size_t I, char... Ts> struct nth_char {
  constexpr static char value =
      select<I>(indexer<std::make_index_sequence<sizeof...(Ts)>, Ts...>{});
};

// Runtime/compile-time string comparison
template<typename Indices, char... Pack>
struct compare_indices;

template<size_t ...Indices, char... Pack>
struct compare_indices<std::index_sequence<Indices...>, Pack...> {
  constexpr static auto helper(const char* value) {
    return ((value[Indices] != 0 && value[Indices] == Pack) && ...);
  }
};

template<typename P>
struct compare_helper;

template<char... Pack>
struct compare_helper<string_literal<Pack...>>
  : compare_indices<std::make_index_sequence<sizeof...(Pack)>, Pack...> {};

template<typename I, typename P, char... Pack2>
struct static_compare_helper;

template<size_t... i, char... Pack, char...Pack2>
struct static_compare_helper<std::index_sequence<i...>, string_literal<Pack...>, Pack2...> {
  static constexpr bool compare() {
    if constexpr (sizeof...(Pack2) != sizeof...(Pack)) {
      return false;
    } else {
      return ((nth_char<i, Pack2...>::value
            == nth_char<i, Pack...>::value) && ...);
    }
  }
};

template<typename Pack, size_t N>
constexpr bool compare(const char value[N]) {
  if constexpr (string_length<Pack>{} != N) {
    return false;
  } else {
    return compare_helper<Pack>::helper(value);
  }
}

template<typename Pack>
constexpr bool compare(const char* value) {
  if (value == NULL) {
    return false;
  }
  return compare_helper<Pack>::helper(value);
}

template<typename Pack, char...Pack2>
constexpr bool compare(const string_literal<Pack2...>& value) {
  return static_compare_helper<std::make_index_sequence<sizeof...(Pack2)>, Pack, Pack2...>::compare();
}

}  // namespace string_literal
}  // namespace jk
