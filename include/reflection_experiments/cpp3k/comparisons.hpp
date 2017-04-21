#pragma once

#include "refl_utilities.hpp"

namespace reflcompare {

namespace meta = std::meta;
namespace refl = jk::refl_utilities;

template<typename T>
bool equal(const T& a, const T& b) {
  if constexpr (refl::is_detected<refl::equality_comparable, T>{}) {
    return a == b;
  } else {
    bool result = true;
    meta::for_each($T.member_variables(),
      [&a, &b](auto&& member){
        constexpr auto ptr = member.pointer();
        result &= equal(a.*ptr, b.*ptr);
      }
    );
    return result;
  }
}

}  // namespace reflcompare
