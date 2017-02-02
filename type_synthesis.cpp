/* Type synthesis with reflexpr proof of concept
 *
 * This example assumes default-constructible parameters.
 * */

#include <iostream>
#include <reflexpr>
#include <tuple>

/* Metaprogramming utilities */

template <char... Pack>
using char_sequence = std::integer_sequence<char, Pack...>;

template <char...> struct string_literal {};

template <typename charT, charT... Pack>
constexpr string_literal<Pack...> operator""_s() {
  return {};
}

// adapted from http://ldionne.com/2015/11/29/efficient-parameter-pack-indexing/
template <std::size_t I, char T> struct indexed {
  constexpr static char value = T;
};

template <typename Is, char... Ts> struct indexer;

template <std::size_t... Is, char... Ts>
struct indexer<std::index_sequence<Is...>, Ts...> : indexed<Is, Ts>... {};

template <std::size_t I, char T> constexpr static char select(indexed<I, T>) {
  return indexed<I, T>::value;
}

template <std::size_t I, char... Ts> struct nth_char {
  constexpr static char value =
      select<I>(indexer<std::make_index_sequence<sizeof...(Ts)>, Ts...>{});
};

// compare ...Pack to a const char[N]
template <char... Pack> struct string_ops {
  template <typename CharArray, std::size_t... i>
  constexpr static bool equal_helper(std::index_sequence<i...>,
                                     CharArray &&value) {
    return ((value[i] == nth_char<i, Pack...>::value) && ...);
  }

  template <char... ComparePack, std::size_t... i>
  constexpr static bool equal_helper(std::index_sequence<i...>,
                                     const string_literal<ComparePack...> &) {
    if constexpr(sizeof...(ComparePack) != sizeof...(Pack)) {
      return false;
    } else {
      return ((nth_char<i, ComparePack...>::value
            == nth_char<i, Pack...>::value) && ...);
    }
  }

  template <typename CharArray> constexpr static bool equal(CharArray &&value) {
    return equal_helper(std::make_index_sequence<sizeof...(Pack)>{}, value);
  }
};

template <typename KeyT, char... Pack> struct associative_access {
  template <size_t... i>
  constexpr static std::size_t access_helper(std::index_sequence<i...> &&) {
    static_assert((string_ops<Pack...>::equal(std::get<i>(KeyT::value)) || ...),
                  "Key index not found in synthesized type");
    return ((string_ops<Pack...>::equal(std::get<i>(KeyT::value)) ? i : 0) +
            ...);
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

template <typename T, typename Literal> struct remove_at_key;

template <typename T, char... Pack>
struct remove_at_key<T, string_literal<Pack...>> {
  constexpr static auto removed_index =
      associative_access<typename T::key_type, Pack...>::value;
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

  template <char... Pack, size_t... i>
  struct check_helper<string_literal<Pack...>, std::index_sequence<i...>> {
    static_assert(!(string_ops<Pack...>::equal(std::get<i>(ObjKeys::value)) ||
                    ...),
                  "Cannot add member which is already in type.");
  };

  constexpr static auto check = check_helper<
      std::tuple_element_t<0, T>,
      std::make_index_sequence<std::tuple_size_v<typename ObjKeys::type>>>{};

  constexpr static auto value = std::tuple_cat(T{}, ObjKeys::value);
  using type = std::decay_t<decltype(value)>;
};

template <typename KeyT, typename ValueT> struct labeled_container {
  template <char... Pack> auto &get(string_literal<Pack...> &&) {
    return std::get<associative_access<KeyT, Pack...>::value>(values);
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

  template <typename... Members>
  using tuple_helper = std::tuple<
      std::meta::get_reflected_type_t<std::meta::get_type_m<Members>>...>;
  using tuple_type =
      std::meta::unpack_sequence_t<std::meta::get_data_members_m<MetaObj>,
                                   tuple_helper>;

  template <char... Pack> auto &get(string_literal<Pack...> &&p) {
    // delegate
    return members.get(std::forward<string_literal<Pack...>>(p));
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
};
template <typename T> struct synthesize_type<T> {
  using result = synthesized_type<T>;
};

/* Operations for synthesizing types
 * */
// Add a member of type T and name Name
//  TODO: Static assert that member is not already in the field
template <typename Field, typename PackIndex> struct add_member {

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

struct old_type {
  int foo;
  float baz;
};

int main() {
  using new_type =
      synthesize_type<old_type, add_member<std::string, decltype("bar"_s)>,
                      remove_member<decltype("baz"_s)>>::result;

  // Does not compile: cannot remove qux because it is not a member of the original type.
  // using new_type = synthesize_type<old_type, remove_member<decltype("qux"_s)>>::result;

  // Does not compile: foo is already a member of the original type, we can't add it.
  // using new_type = synthesize_type<old_type, add_member<char, decltype("foo"_s)>>::result;

  new_type x;

  auto foo_v = access(x, "foo"_s);
  static_assert(std::is_same_v<std::decay_t<decltype(foo_v)>, int>);

  auto bar_v = access(x, "bar"_s);
  static_assert(std::is_same_v<std::decay_t<decltype(bar_v)>, std::string>);

  // Does not compile: baz was removed from the synthesized type
  // access(x, "baz"_s);

  // Does not compile: qux is not a member of the synthesized type
  // access(x, "qux"_s);
}
