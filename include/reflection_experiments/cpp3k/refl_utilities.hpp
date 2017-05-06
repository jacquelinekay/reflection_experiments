#pragma once

#include "../meta_utilities.hpp"
#include "../string_literal.hpp"
#include <cpp3k/meta>

#include <cpp3k/detail/tuple.hpp>

namespace jk {
namespace refl_utilities {

namespace meta = cpp3k::meta;
namespace metap = jk::metaprogramming;
namespace sl = jk::string_literal;

template<typename T>
using has_member_variables = std::void_t<decltype($T.member_variables())>;

template<typename T>
static constexpr bool is_member_type() {
  return metap::is_detected<has_member_variables, T>{};
}

template<typename T, typename StrT>
static constexpr bool has_member(StrT&& key) {
  bool has_member = false;
  meta::for_each($T.member_variables(),
    [&key, &has_member](auto&& member) {
      if constexpr (sl::equal(key, member.type_name())) {
        has_member = true;
      }
    }
  );
  return has_member;
}

template<typename S, typename Member>
using unreflect_member_t = typename std::decay_t<
    decltype(std::declval<S>().*(std::decay_t<Member>::pointer()))>;

}  // namespace refl_utilities
}  // namespace jk
