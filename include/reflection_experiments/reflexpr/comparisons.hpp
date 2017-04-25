#pragma once

#include "../meta_utilities.hpp"
#include "refl_utilities.hpp"

// TODO: generalize for different binary operators and the fold operator

namespace reflcompare {

namespace meta = std::meta;
namespace refl = jk::refl_utilities;
namespace metap = jk::metaprogramming;

template<typename T>
bool equal(const T& a, const T& b);

template<typename ...MetaMembers>
struct compare_fold {
  template<typename T>
  static constexpr auto apply(const T& a, const T& b) {
    return (equal(a.*meta::get_pointer<MetaMembers>::value,
                  b.*meta::get_pointer<MetaMembers>::value) && ...);
  }
};

template<typename T>
bool equal(const T& a, const T& b) {
  using MetaT = reflexpr(T);
  if constexpr (metap::is_detected<metap::equality_comparable, T>{}) {
    return a == b;
  } else {
    static_assert(meta::Record<MetaT>,
      "Type contained a member which has no comparison operator defined.");
    return meta::unpack_sequence_t<meta::get_data_members_m<MetaT>, compare_fold>::apply(a, b);
  }
}

}  // namespace reflcompare
