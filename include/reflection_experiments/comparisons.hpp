#pragma once

#include "meta_utilities.hpp"
#include "refl_utilities.hpp"


namespace reflcompare {

namespace refl = jk::refl_utilities;
namespace metap = jk::metaprogramming;
#if USING_REFLEXPR
namespace meta = std::meta;

template<typename T>
bool equal(const T& a, const T& b);

template<typename ...MetaMembers>
struct compare_fold {
  template<typename T>
  static constexpr auto apply(const T& a, const T& b) {
    return (equal(
      a.*meta::get_pointer<MetaMembers>::value,
      b.*meta::get_pointer<MetaMembers>::value) && ...);
  }
};
#elif USING_CPP3K
namespace meta = cpp3k::meta;
#endif

template<typename T>
bool equal(const T& a, const T& b) {
  if constexpr (metap::is_detected<metap::equality_comparable, T>{}) {
    return a == b;
  } else if constexpr (metap::is_detected<metap::iterable, T>{}) {
    bool equals = true;
    if (a.size() != b.size()) {
      return false;
    }
    for (int i = 0; i < a.size(); ++i) {
      equals &= equal(a[i], b[i]);
    }
    return equals;
  } else {
#if USING_REFLEXPR
    using MetaT = reflexpr(T);
    static_assert(meta::Record<MetaT>,
      "Type contained a member which has no comparison operator defined.");
    return meta::unpack_sequence_t<meta::get_data_members_m<MetaT>, compare_fold>::apply(a, b);
#elif USING_CPP3K
    bool result = true;
    meta::for_each($T.member_variables(),
      [&a, &b, &result](auto&& member){
        result &= equal(a.*member.pointer(), b.*member.pointer());
      }
    );
    return result;
#endif
  }
}

}  // namespace reflcompare
