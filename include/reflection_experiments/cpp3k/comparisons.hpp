#pragma once

#include "../meta_utilities.hpp"
#include "refl_utilities.hpp"

namespace reflcompare {

namespace meta = cpp3k::meta;
namespace refl = jk::refl_utilities;
namespace metap = jk::metaprogramming;

template<typename T>
bool equal(const T& a, const T& b) {
  if constexpr (metap::is_detected<metap::equality_comparable, T>{}) {
    return a == b;
  } else {
    bool result = true;
    meta::for_each($T.member_variables(),
      [&a, &b, &result](auto&& member){
        result &= equal(a.*member.pointer(), b.*member.pointer());
      }
    );
    return result;
  }
}

}  // namespace reflcompare
