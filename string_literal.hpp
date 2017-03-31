#pragma once

template <char...> struct string_literal {};

template <typename charT, charT... Pack>
constexpr string_literal<Pack...> operator""_s() {
  return {};
}

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

// compare ...Pack to a const char[N]
template <char... Pack> struct string_ops {
  template <typename CharArray, std::size_t... i>
  constexpr static bool equal_helper(std::index_sequence<i...>,
                                     CharArray &&value) {
    return ((value[i] == nth_char<i, Pack...>::value) && ...);
  }

  template <char... ComparePack, std::size_t... i>
  constexpr static bool equal_helper(std::index_sequence<i...>,
                                     const string_literal<ComparePack...> &) {
    if constexpr(sizeof...(ComparePack) != sizeof...(Pack)) {
      return false;
    } else {
      return ((nth_char<i, ComparePack...>::value
            == nth_char<i, Pack...>::value) && ...);
    }
  }

  template <typename CharArray> constexpr static bool equal(CharArray &&value) {
    return equal_helper(std::make_index_sequence<sizeof...(Pack)>{}, value);
  }
};

template<typename T>
struct unpack_string_literal;

template<char ...Pack>
struct unpack_string_literal<string_literal<Pack...>> : string_ops<Pack...> {
};


// Runtime/compile-time string comparison

template<typename Indices, char... Pack>
struct compare_indices;

template<size_t ...Indices, char... Pack>
struct compare_indices<std::index_sequence<Indices...>, Pack...> {
  static auto helper(char* value) {
    return ((value[Indices] != 0 && value[Indices] == Pack) && ...);
  }
};

template<typename P>
struct compare_helper;

template<char... Pack>
struct compare_helper<string_literal<Pack...>>
  : compare_indices<std::make_index_sequence<sizeof...(Pack)>, Pack...> { };

template<typename Pack>
bool compare(char* value) {
  if (value == NULL) {
    return false;
  }
  return compare_helper<Pack>::helper(value);
}
