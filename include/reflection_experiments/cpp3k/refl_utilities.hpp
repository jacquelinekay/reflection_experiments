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
constexpr static bool is_member_type() {
  return metap::is_detected<has_member_variables, T>{};
}

template<typename T, typename StrT>
constexpr static bool has_member(StrT&& key) {
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

template<typename T>
struct get_matching_index {
  template<typename Id, std::size_t ...I>
  constexpr static std::size_t apply(Id&& id, std::index_sequence<I...>) {
    return ((sl::equal(id, cpp3k::meta::cget<I>($T.member_variables()).name()) ? I : 0) + ...);
  }
};

template<typename T, typename Id>
constexpr static auto get_metainfo_for(Id&& id) {
  return cpp3k::meta::cget<
      get_matching_index<T>::apply(Id{}, std::make_index_sequence<$T.member_variables().size()>{})
    >($T.member_variables());
}

template<typename S, typename Member>
struct unreflect_member {
  using type = std::decay_t<decltype(std::declval<S>().*(std::decay_t<Member>::pointer()))>;
};

template<typename S, typename Member>
using unreflect_member_t = typename unreflect_member<S, Member>::type;

}  // namespace refl_utilities
}  // namespace jk
