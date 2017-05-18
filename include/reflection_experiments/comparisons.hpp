#pragma once

#include "meta_utilities.hpp"
#include "refl_utilities.hpp"


namespace reflcompare {

namespace refl = jk::refl_utilities;
namespace metap = jk::metaprogramming;
#if USING_REFLEXPR
namespace meta = std::meta;
#elif USING_CPP3K
namespace meta = cpp3k::meta;
#endif

template<typename T>
bool equal(const T& a, const T& b) {
  if constexpr (metap::is_detected<metap::equality_comparable, T>{}) {
    return a == b;
  } else if constexpr (metap::is_detected<metap::iterable, T>{}) {
    if (a.size() != b.size()) {
      return false;
    }
    for (int i = 0; i < a.size(); ++i) {
      if (!equal(a[i], b[i])) {
        return false;
      }
    }
    return true;
  } else {
#if USING_REFLEXPR
    using MetaT = reflexpr(T);
    static_assert(meta::Record<MetaT>,
      "Type contained a member which has no comparison operator defined.");
    bool result = true;
    meta::for_each<meta::get_data_members_m<MetaT>>(
      [&a, &b, &result](auto&& member) {
        using MetaMember = typename std::decay_t<decltype(member)>;
        constexpr auto p = meta::get_pointer<MetaMember>::value;
        result = result && equal(a.*p, b.*p);
      }
    );
    return result;
#elif USING_CPP3K
    bool result = true;
    meta::for_each($T.member_variables(),
      [&a, &b, &result](auto&& member){
        result = result && equal(a.*member.pointer(), b.*member.pointer());
      }
    );
    return result;
#endif
  }
}

}  // namespace reflcompare
