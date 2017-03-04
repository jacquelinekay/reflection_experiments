#pragma once

template <char...> struct string_literal {};

template <typename charT, charT... Pack>
constexpr string_literal<Pack...> operator""_s() {
  return {};
}

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
