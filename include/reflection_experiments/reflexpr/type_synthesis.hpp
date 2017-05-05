#pragma once

#include <reflexpr>
#include <tuple>

#include "refl_utilities.hpp"
#include "string_literal.hpp"

namespace type_synthesis {

namespace refl = jk::refl_utilities;
namespace sl = jk::string_literal;

template <typename KeyT, typename Name>
struct associative_access {
  template <size_t... i>
  constexpr static std::size_t access_helper(std::index_sequence<i...> &&) {
    static_assert((sl::compare<Name>(std::get<i>(KeyT::value)) || ...),
                  "Key index not found in synthesized type");
    return ((sl::compare<Name>(std::get<i>(KeyT::value)) ? i : 0) + ...);
  }

  constexpr static std::size_t value = access_helper(
      std::make_index_sequence<std::tuple_size_v<typename KeyT::type>>());
};

template <typename T, typename I> struct get_element_until;

template <typename T, size_t... i>
struct get_element_until<T, std::index_sequence<i...>> {
  using type = std::tuple<std::tuple_element_t<i, T>...>;

  constexpr static auto slice(const T &sliced) {
    return std::make_tuple(std::get<i>(sliced)...);
  }
};

template <typename T, size_t Skip, typename I> struct get_element_after;

template <typename T, size_t Skip, size_t... i>
struct get_element_after<T, Skip, std::index_sequence<i...>> {
  using type = std::tuple<std::tuple_element_t<i + Skip + 1, T>...>;

  constexpr static auto slice(const T &sliced) {
    return std::make_tuple(std::get<i + Skip + 1>(sliced)...);
  }
};

template <typename OriginalKeyT, size_t RemoveIndex> struct remove_key_wrapper {
  using TupleT = typename OriginalKeyT::type;
  constexpr static std::size_t N = std::tuple_size_v<TupleT>;

  constexpr static auto value = std::tuple_cat(
      get_element_until<TupleT, std::make_index_sequence<RemoveIndex>>::slice(
          OriginalKeyT::value),
      get_element_after<TupleT, RemoveIndex,
                        std::make_index_sequence<N - RemoveIndex - 1>>::
          slice(OriginalKeyT::value));
  using type = std::decay_t<decltype(value)>;
};

template <typename T, typename Name>
struct remove_at_key {
  constexpr static auto removed_index =
      associative_access<typename T::key_type, Name>::value;
  constexpr static auto N = std::tuple_size_v<typename T::tuple_type>;

  using values = std::decay_t<decltype(
      std::tuple_cat(std::declval<typename get_element_until<
                         typename T::tuple_type,
                         std::make_index_sequence<removed_index>>::type>(),
                     std::declval<typename get_element_after<
                         typename T::tuple_type, removed_index,
                         std::make_index_sequence<N - removed_index - 1>>::type>()))>;

  using keys = remove_key_wrapper<typename T::key_type, removed_index>;
};

template <typename MetaObj> struct object_keys {
  template <typename... MetaArgs> struct keys_helper {
    constexpr static auto keys =
        std::make_tuple(std::meta::get_base_name_v<MetaArgs>...);
  };

  constexpr static auto value =
      std::meta::unpack_sequence_t<std::meta::get_data_members_m<MetaObj>,
                                   keys_helper>::keys;
  using type = std::decay_t<decltype(value)>;
};

template <typename T, typename ObjKeys> struct key_wrapper {
  // Unpack the first member of T
  template <typename P, typename I> struct check_helper;

  template <typename Name, size_t... i>
  struct check_helper<Name, std::index_sequence<i...>> {
    static_assert(!(sl::compare<Name>(std::get<i>(ObjKeys::value)) ||
                    ...),
                  "Cannot add member which is already in type.");
  };

  constexpr static auto check = check_helper<
      std::tuple_element_t<0, T>,
      std::make_index_sequence<std::tuple_size_v<typename ObjKeys::type>>>{};

  constexpr static auto value = std::tuple_cat(T{}, ObjKeys::value);
  using type = std::decay_t<decltype(value)>;
};

template <typename KeyT, typename ValueT>
struct labeled_container {
  template <typename Name>
  auto &get(Name &&) {
    return std::get<associative_access<KeyT, Name>::value>(values);
  }

  constexpr static auto keys = KeyT::value;
  ValueT values;

  using key_type = KeyT;
  using value_type = ValueT;
};

/* Type synthesis utilities
 * */

// base case
template <typename T> struct synthesized_type {
  using MetaObj = reflexpr(T);
  using tuple_type =
    std::meta::unpack_sequence_t<std::meta::get_data_members_m<MetaObj>,
                                 refl::member_pack_as_tuple>;

  template <char... Pack> auto &get(sl::string_literal<Pack...> &&p) {
    // delegate
    return members.get(std::forward<sl::string_literal<Pack...>>(p));
  }

  // Now, we must operate on the members.
  labeled_container<object_keys<MetaObj>, tuple_type> members;

  using key_type = typename decltype(members)::key_type;
  using value_type = tuple_type;
};

template <typename T, typename... Operations> struct synthesize_type;

// TODO: recursion is bad for compile time performance
template <typename T, typename Operator, typename... Operations>
struct synthesize_type<T, Operator, Operations...> {
  using result = typename Operator::template apply<
      typename synthesize_type<T, Operations...>::result>;

  constexpr synthesize_type(Operator&&, Operations&&...) { }
};

template <typename T> struct synthesize_type<T> {
  using result = synthesized_type<T>;
};

/* Operations for synthesizing types
 * */
// Add a member of type T and name Name
template <typename FieldType> struct add_member {
  constexpr add_member(const string_literal& name) {
    // TODO
  };

  template <typename T, typename TupleT> struct meta_tuple_cat;

  template <typename T, typename... TupleArgs>
  struct meta_tuple_cat<T, std::tuple<TupleArgs...>> {
    using type = std::tuple<T, TupleArgs...>;
  };

  template <typename T>
  using apply = labeled_container<
      key_wrapper<std::tuple<PackIndex>, typename T::key_type>,
      typename meta_tuple_cat<Field, typename T::value_type>::type>;
};

template <typename PackIndex> struct remove_member {
  template <typename T> using removed = remove_at_key<T, PackIndex>;

  // compile time error if field is not in T
  template <typename T>
  using apply =
      labeled_container<typename removed<T>::keys, typename removed<T>::values>;
};

template <typename T, typename S> decltype(auto) access(T &&t, S &&s) {
  return t.get(std::forward<S>(s));
}


}  // namespace type_synthesis
