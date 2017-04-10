#pragma once
#include "string_literal.hpp"
#include <reflexpr>

namespace jk {

namespace refl_utilities {

namespace sl = string_literal;
namespace meta = std::meta;

// from http://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
template<typename Test, template<typename...> class Ref>
struct is_specialization : std::false_type {};

template<template<typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

// TODO
template<typename S>
struct is_iterable {
  template<typename T>
  static constexpr bool test(T* pt, decltype(pt->begin())* = nullptr, decltype(pt->end())* = nullptr,
      typename T::iterator* = nullptr) {
    return std::is_same<decltype(pt->begin()), typename T::iterator>::value &&
                std::is_same<decltype(pt->end()), typename T::iterator>::value;
  }

  template<typename T>
  static constexpr bool test(...) {
    return false;
  }

  static constexpr const bool value = test<S>(nullptr);
};

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
  template<typename T, typename Func>
  static inline auto apply(T&& t, Func&& func) {
    // TODO: Nope.
    return (func(Object{}, t.*meta::get_pointer<Object>{}), ...);
  }
};

template<typename T, typename Func>
auto meta_fold_over_data_members(T&& t, Func&& func) {
  using MetaT = reflexpr(T);
	return meta::unpack_sequence_t<meta::get_data_members_m<MetaT>, runtime_fold_helper>::apply(t, func);
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

template<typename T, typename MetaInfo>
struct index_of_metainfo : std::integral_constant<size_t, index_metainfo_helper<T, MetaInfo>(
  std::make_index_sequence<n_fields<T>{}>{})> {};

template<typename T, typename MemberName>
struct get_member_pointer : meta::get_pointer<meta::get_element_m<
                            meta::get_data_members_m<reflexpr(T)>,
                            index_of_member<T, MemberName>{}>> { };

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

// implement overload
template<typename... Ts>
struct overloader {
  using Ts::operator()...;

  overloader(Ts... xs) : fs(xs...) {}

  std::tuple<Ts...> fs;
};

template <typename... Ts>
auto overload(Ts&... xs)
{
  return overloader<std::decay_t<Ts>...>{xs...};
}

// adapted from http://stackoverflow.com/questions/35608977/understanding-y-combinator-through-generic-lambdas
template<class F>
struct y_combinator_t {
  F f;
  template<class...Args>
  decltype(auto) operator()(Args&&...args)const {
    return f(*this, std::forward<Args>(args)...);
  }
};

template<class F>
y_combinator_t<std::decay_t<F>> y_combinator( F&& f ) {
  return {std::forward<F>(f)};
};

}  // namespace refl_utilities

}  // namespace jk
