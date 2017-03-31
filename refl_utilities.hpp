#pragma once
#include "string_literal.hpp"

namespace jk {

namespace refl_utilities {

namespace sl = jk::string_literal;
namespace meta = std::meta;

// from http://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
template<typename Test, template<typename...> class Ref>
struct is_specialization : std::false_type {};

template<template<typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

template<typename ...Types>
struct reduce_pack {
  // Find the type in the pack which is non-void

  template <typename Tn, void* ...v>
  static Tn f(decltype(v)..., Tn, ...);

  using type = decltype(f(std::declval<Types>()...));
};

template <typename... Members>
using member_pack_as_tuple = std::tuple<
  meta::get_reflected_type_t<meta::get_type_m<Members>>...>;

template<typename T>
struct n_fields : meta::get_size<meta::get_data_members_m<reflexpr(T)>> {};

template<typename T, typename MemberName>
struct has_member {
  using MetaObj = meta::get_aliased_m<reflexpr(T)>;

  template<typename ...MetaField>
  struct has_member_pack {
    static constexpr bool value = ((sl::unpack_string_literal<MemberName>::equal(
      meta::get_base_name_v<MetaField>)) || ...);
  };

  static constexpr bool value = meta::unpack_sequence_t<
    meta::get_member_types_m<MetaObj>, has_member_pack>::value;
};

template<typename T, typename MemberName>
struct get_member {
  using MetaObj = reflexpr(T);

  template<typename ...MetaField>
  struct get_member_pack {
    using field = typename reduce_pack<
      typename std::conditional<
        sl::unpack_string_literal<MemberName>::equal(meta::get_base_name_v<MetaField>),
          MetaField, void*
      >::type...
    >::type;
  };
  // get the data member with the requested name
  using MetaField = typename meta::unpack_sequence_t<
    meta::get_data_members_m<MetaObj>, get_member_pack>::field;
};

template<typename T, typename MemberName>
struct index_of_member {
  using MetaObj = reflexpr(T);

  template<std::size_t ...i>
  constexpr static std::size_t index_helper(std::index_sequence<i...>) {
    return ((sl::unpack_string_literal<MemberName>::equal(
      meta::get_base_name_v<
        meta::get_element_m<
          meta::get_data_members_m<MetaObj>, i>>) ? i : 0) + ...);
  }

  constexpr static std::size_t index = index_helper(std::make_index_sequence<n_fields<T>::value>{});
};

template<typename T, typename MemberName>
struct get_member_pointer {
  using MetaObj = reflexpr(T);
  using Element = meta::get_element_m<
    meta::get_data_members_m<MetaObj>, index_of_member<T, MemberName>::index>;

  static constexpr auto value = meta::get_pointer<Element>::value;
};

template<typename T, typename MemberName>
struct get_member_type {
  using MetaObj = reflexpr(T);
  using type = meta::get_reflected_type_t<meta::get_type_m<meta::get_element_m<
    meta::get_data_members_m<MetaObj>, index_of_member<T, MemberName>::index>>>;
};

}  // namespace refl_utilities

}  // namespace jk
