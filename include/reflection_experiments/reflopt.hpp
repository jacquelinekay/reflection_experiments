#pragma once

#include <boost/hana/at_key.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/plus.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>

#include <boost/lexical_cast.hpp>

#include <experimental/optional>

#include "refl_utilities.hpp"
#include "meta_utilities.hpp"

#include <boost/hana/length.hpp>

#if USING_REFLEXPR
#elif USING_CPP3K
#include "cpp3k/adapt_hana.hpp"
#else
NO_REFLECTION_IMPLEMENTATION_FOUND()
#endif

namespace reflopt {
  static const size_t max_value_length = 128;

  namespace refl = jk::refl_utilities;
  namespace metap = jk::metaprogramming;
  namespace hana = boost::hana;
  using namespace hana::literals;

  template<typename T>
  using optional_t = std::experimental::optional<T>;

  // Compare a hana string to a const char*
  template<typename Str>
  bool runtime_string_compare(const Str&, const char* x) {
    return strcmp(hana::to<const char*>(Str{}), x) == 0;
  }

#if USING_REFLEXPR
  namespace meta = std::meta;
  template<typename T, size_t... I>
  static constexpr auto get_name_helper(std::index_sequence<I...>) {
    return hana::string_c<meta::get_base_name_v<T>[I]...>;
  }

  template<typename T>
  static constexpr auto get_name() {
    constexpr auto L = metap::cstrlen(meta::get_base_name_v<T>);
    return get_name_helper<T>(std::make_index_sequence<L>{});
  }
#elif USING_CPP3K
  namespace meta = cpp3k::meta;

  template<typename T, size_t... I>
  static constexpr auto get_name_helper(std::index_sequence<I...>) {
    return hana::string_c<T::name()[I]...>;
  }

  template<typename T>
  static constexpr auto get_name() {
    constexpr auto L = metap::cstrlen(T::name());
    return get_name_helper<T>(std::make_index_sequence<L>{});
  }
#endif

  template<typename OptionsStruct>
  struct OptionsMap {
    static constexpr auto collect_flags = [](auto&& flag_map, auto&& field) {
      using T = std::decay_t<decltype(field)>;

      constexpr auto field_name = get_name<T>();
      using FlagMap = std::decay_t<decltype(flag_map)>;
      constexpr auto flag = "--"_s + field_name;
      constexpr auto result = hana::insert(FlagMap{}, hana::make_pair(flag, T{}));
      using Result = std::decay_t<decltype(result)>;

      constexpr auto prefix = hana::drop_while(field_name,
        [](auto elem) {
          return hana::contains(Result{}, "-"_s + hana::string_c<elem>);
        }
      );

      static_assert(!hana::is_empty(prefix), "Couldn't find unique short flag.");

      return hana::insert(result,
          hana::make_pair("-"_s + hana::string_c<hana::front(prefix)>, T{}));
    };

#if USING_REFLEXPR
    using MetaOptions = reflexpr(OptionsStruct);
    template<typename... MetaFields>
    struct make_prefix_map {
      static constexpr auto helper() {
        return hana::fold(
          hana::make_tuple(MetaFields{}...),
          hana::make_map(),
          collect_flags
        );
      }
    };

    static constexpr auto prefix_map = meta::unpack_sequence_t<
      meta::get_data_members_m<MetaOptions>, make_prefix_map>::helper();

#elif USING_CPP3K
    static constexpr auto prefix_map = hana::fold(
      refl::adapt_to_hana($OptionsStruct.member_variables()),
      hana::make_map(),
      collect_flags
    );
#endif

    static_assert(!hana::length(hana::keys(prefix_map)) == hana::size_c<0>);

    static bool contains(const char* prefix) {
      return hana::fold(hana::keys(prefix_map),
        false,
        [&prefix](bool x, auto&& key) {
          return x || runtime_string_compare(key, prefix);
        }
      );
    }

    static auto set(OptionsStruct& options, const char* prefix, const char* value) {
      hana::for_each(hana::keys(prefix_map),
        [&options, &prefix, &value](auto&& key) {
          if (runtime_string_compare(key, prefix)) {
            constexpr auto info = hana::at_key(prefix_map, std::decay_t<decltype(key)>{});
#if USING_REFLEXPR
            using MetaInfo = std::decay_t<decltype(info)>;
            constexpr auto member_pointer = meta::get_pointer<MetaInfo>::value;
            using MemberType = meta::get_reflected_type_t<meta::get_type_m<MetaInfo>>;
#elif USING_CPP3K
            constexpr auto member_pointer = info.pointer();
            using MemberType = refl::unreflect_member_t<OptionsStruct, decltype(info)>;
#endif
            options.*member_pointer = boost::lexical_cast<MemberType>(
              value, strnlen(value, max_value_length));
          }
        }
      );
    }
  };

  // ArgVT boilerplate is to enable both char** and const char*[]'s for testing
  template<typename OptionsStruct, typename ArgVT,
    typename std::enable_if_t<
      std::is_same<ArgVT, char**>{} || std::is_same<ArgVT, const char**>{}>* = nullptr
  >
  optional_t<OptionsStruct> parse(int argc, ArgVT const argv) {
    OptionsStruct options;
    for (int i = 1; i < argc; i += 2) {
      if (OptionsMap<OptionsStruct>::contains(argv[i])) {
          OptionsMap<OptionsStruct>::set(options, argv[i], argv[i + 1]);
      } else {
        // unknown prefix found
        return std::experimental::nullopt;
      }
    }

    return options;
  }

}  // namespace reflopt
