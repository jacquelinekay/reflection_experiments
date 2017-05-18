#pragma once

#include <boost/hana/tuple.hpp>
#include <cpp3k/meta>

namespace jk {
namespace refl_utilities {
namespace hana = boost::hana;

template<typename T, size_t ...I>
constexpr auto adapt_to_hana_helper(const T& t, std::index_sequence<I...>&&) {
  return hana::make_tuple(
    cpp3k::meta::v1::cget<I>(t)...
  );
}

template<typename T>
constexpr auto adapt_to_hana(T&& t) {
  // return std::apply(hana::make_tuple, std::forward<T>(t));
  return adapt_to_hana_helper(t, std::make_index_sequence<T::size()>{});
}

template<size_t ...I>
constexpr static auto make_index_sequence_tuple_helper(std::index_sequence<I...>&&) {
  return hana::make_tuple(hana::int_c<I>...);
}

template<size_t N>
constexpr static auto make_index_sequence_tuple() {
  return make_index_sequence_tuple_helper(std::make_index_sequence<N>{});
}

}
}
