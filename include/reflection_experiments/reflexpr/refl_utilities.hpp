#pragma once
#include "../string_literal.hpp"
#include <reflexpr>

#include <experimental/type_traits>
#include <variant>

namespace jk {
namespace refl_utilities {

namespace sl = string_literal;
namespace meta = std::meta;

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

// generic meta-object fold
template<typename ...Object>
struct runtime_fold_helper {
  template<typename T, typename Init, typename Func>
  static inline auto apply(T&& t, Init&& init, Func&& func) {
    return fold(func, init, t.*meta::get_pointer<Object>::value ...);
  }
};

template<typename ...MetaField>
struct has_member_pack {
  template<typename StrT>
  static constexpr bool apply(const StrT& name) {
    return (sl::equal(name, meta::get_base_name_v<MetaField>) || ...);
  }
};

template<typename T, typename StrT>
constexpr bool has_member(const StrT& member_name) {
  return meta::unpack_sequence_t<
    meta::get_member_types_m<reflexpr(T)>, has_member_pack
  >::apply(member_name);
}

template<typename T, auto Str, std::size_t ...I>
static constexpr auto index_helper(std::index_sequence<I...>) {
  return ((Str ==
            meta::get_base_name_v<
              meta::get_element_m<
                meta::get_data_members_m<reflexpr(T)>, I>
              > ? I : 0
          ) + ...);
}

template<typename T, auto Str>
static constexpr auto index_of_member() {
  return index_helper<T, Str>(std::make_index_sequence<n_fields<T>{}>{});
}

template<typename T, auto Str>
constexpr auto get_member_pointer() {
  return meta::get_pointer<
    meta::get_element_m<
      meta::get_data_members_m<reflexpr(T)>,
      index_of_member<T, Str>()
    >
  >::value;
}

// free metafunctions for metaobjects
struct get_name {
  template<typename MetaT>
  constexpr auto operator()(MetaT&& t) {
    return meta::get_base_name<MetaT>{};
  }
};

}  // namespace refl_utilities
}  // namespace jk
