#pragma once
#include "string_literal.hpp"
#include "generalized_fold.hpp"
#include <reflexpr>

#include <experimental/type_traits>
#include <variant>

namespace jk {

namespace refl_utilities {

namespace sl = string_literal;
namespace meta = std::meta;

// from http://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
template<typename Test, template<typename...> class Ref>
struct is_specialization : std::false_type {};

template<template<typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

template<typename T>
using stringable = std::void_t<decltype(std::to_string(std::declval<T>()))>;

template<typename T>
using iterable = std::void_t<
  decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>;

template<typename T>
using resizable = std::void_t<decltype(std::declval<T>().resize())>;

template<typename T>
using has_tuple_size = std::void_t<std::tuple_size<T>>;

template<typename T>
using equality_comparable = std::void_t<decltype(std::declval<T>() == std::declval<T>())>;

template<template<typename ...> typename Op, typename... Args>
using is_detected = std::experimental::is_detected<Op, Args...>;

// Unwrap type from hana::type_c
#define UNWRAP_TYPE(TypeWrapper) typename std::decay_t<decltype(TypeWrapper)>::type

template<typename MetaT>
using unreflect_type = meta::get_reflected_type_t<meta::get_type_m<MetaT>>;

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

template<typename MemberName, typename ...MetaField>
struct has_member_pack : std::bool_constant<((sl::compare<MemberName>(
    meta::get_base_name_v<MetaField>)) || ...)> { };

// generic meta-object fold
template<typename ...Object>
struct runtime_fold_helper {
  template<typename T, typename Init, typename Func>
  static inline auto apply(T&& t, Init&& init, Func&& func) {
    return fold(func, init, t.*meta::get_pointer<Object>::value ...);
  }
};

// Func is a callable of arity 2
template<typename T, typename Init, typename Func,
  typename std::enable_if_t<meta::get_size<meta::get_data_members_m<reflexpr(std::decay_t<T>)>>{} >= 1>* = nullptr>
auto fold_over_data_members(Func&& func, Init&& init, T&& t) {
  using MetaT = reflexpr(std::decay_t<T>);
	return meta::unpack_sequence_t<meta::get_data_members_m<MetaT>, runtime_fold_helper>::apply(t, init, func);
}

template<typename T, typename MemberName>
struct has_member {
  template<typename ...MetaFields>
  using curried_has_member_pack = has_member_pack<MemberName, MetaFields...>;

  static constexpr bool value = meta::unpack_sequence_t<
    meta::get_member_types_m<reflexpr(T)>, curried_has_member_pack>::value;
};

template<typename T, typename MemberName>
struct get_member {
  using MetaObj = reflexpr(T);

  template<typename ...MetaField>
  struct get_member_pack {
    using field = typename reduce_pack<
      typename std::conditional<
        sl::compare<MemberName>(meta::get_base_name_v<MetaField>), MetaField, void*>::type...
    >::type;
  };
  // get the data member with the requested name
  using MetaField = typename meta::unpack_sequence_t<
    meta::get_data_members_m<MetaObj>, get_member_pack>::field;
};

template<typename T, typename MemberName, std::size_t ...i>
constexpr static std::size_t index_helper(std::index_sequence<i...>) {
  return ((sl::compare<MemberName>(
    meta::get_base_name_v<
      meta::get_element_m<
        meta::get_data_members_m<reflexpr(T)>, i>>) ? i : 0) + ...);
}

template<typename T, typename MetaInfo, std::size_t ...i>
constexpr static std::size_t index_metainfo_helper(std::index_sequence<i...>) {
  return (((meta::get_base_name_v<MetaInfo> ==
    meta::get_base_name_v<
      meta::get_element_m<
        meta::get_data_members_m<reflexpr(T)>, i>>) ? i : 0) + ...);
}

template<typename T, typename MemberName>
struct index_of_member : std::integral_constant<size_t, index_helper<T, MemberName>(
  std::make_index_sequence<n_fields<T>{}>{})> {};


template<typename T, auto NameT, std::size_t ...i>
constexpr static size_t index_of_helper(std::index_sequence<i...>) {
  using MetaT = reflexpr(T);
  return ((NameT == meta::get_base_name_v<meta::get_element_m<meta::get_data_members_m<MetaT>, i>> ? i : 0) + ...);
}

template<typename T, auto NameT>
static constexpr auto index_of() {
  return index_of_helper<T, NameT>( std::make_index_sequence<n_fields<T>{}>{});
}

template<typename T, typename MetaInfo>
struct index_of_metainfo : std::integral_constant<size_t, index_metainfo_helper<T, MetaInfo>(
  std::make_index_sequence<n_fields<T>{}>{})> {};

template<typename T, typename MemberName>
struct get_member_pointer : meta::get_pointer<meta::get_element_m<
                            meta::get_data_members_m<reflexpr(T)>,
                            index_of_member<T, MemberName>{}>> { };

// TODO
template<typename T, auto NameT>
static constexpr auto member_pointer() {
  return meta::get_pointer<meta::get_element_m<
                          meta::get_data_members_m<reflexpr(T)>,
                          index_of<T, NameT>()>>::value;
}

template<typename T, typename MemberName>
struct get_member_type {
  using type = unreflect_type<meta::get_element_m<
    meta::get_data_members_m<reflexpr(T)>, index_of_member<T, MemberName>{}>>;
};

// free metafunctions for metaobjects
struct get_name {
  template<typename MetaT>
  constexpr auto operator()(MetaT&& t) {
    return meta::get_base_name<MetaT>{};
  }
};


template<typename T>
struct members_variant : std::variant<meta::get_data_members_m<reflexpr(T)> > {};

/*
// return a reference to the field in T that matches the runtime-determined string value
template<typename T>
members_variant<T> get_member(const T& x, const std::string_view& name) {
  // This is a product type-to-sum type mapping
}
*/

// TODO think this is wrong
template<typename T, typename StringT, typename Callable>
void call_for_member(const T& x, const StringT& name, Callable&& callback) {
  auto wrapped_callback = [&name, &callback](auto&& metainfo) {
    using MetaInfo = std::decay_t<decltype(metainfo)>;
    if (name == meta::get_base_name_v<MetaInfo>) {
      callback(metainfo);
    }
  };
  meta::for_each<meta::get_data_members_m<reflexpr(std::decay_t<T>)>>(wrapped_callback);
}

}  // namespace refl_utilities

}  // namespace jk
