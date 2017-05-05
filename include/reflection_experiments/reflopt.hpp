#pragma once

#include <boost/hana/at_key.hpp>
#include <boost/hana/filter.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/string.hpp>
#include <boost/hana/tuple.hpp>

#include <boost/lexical_cast.hpp>

#include <experimental/optional>

#include "refl_utilities.hpp"
#include "meta_utilities.hpp"

#include <vrm/pp/arg_count.hpp>
#include <vrm/pp/cat.hpp>

#include <boost/hana/length.hpp>

#include <iostream>

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

  template<typename T>
  using optional_t = std::experimental::optional<T>;

  // Compare a hana string to a const char*
  template<typename Str>
  bool runtime_string_compare(const Str&, const char* x) {
    return strcmp(hana::to<const char*>(Str{}), x) == 0;
  }

#if USING_REFLEXPR
  namespace meta = std::meta;
  template<typename MetaT>
  struct index_metainfo_helper {
    template<typename Id, size_t I, size_t ...J>
    static constexpr bool equals_member(std::index_sequence<J...>&&) {
      return ((Id{}[hana::size_c<J>] == meta::get_base_name_v<
                  meta::get_element_m<
                    meta::get_data_members_m<MetaT>,
                    I
                  >
                >[J]) && ...);
    }

    template<typename Id, size_t ...Index>
    static constexpr auto apply(Id&&, std::index_sequence<Index...>) {
      return ((equals_member<Id, Index>(
               std::make_index_sequence<hana::length(Id{})>{}) ? Index : 0) + ...);
    }
  };

  template<typename T, typename Id>
  static constexpr auto get_metainfo_for(Id&&) {
    using MetaT = reflexpr(T);
    constexpr auto index = index_metainfo_helper<MetaT>::apply(Id{},
        std::make_index_sequence<refl::n_fields<T>{}>{});
    return meta::get_element_m<meta::get_data_members_m<MetaT>, index>{};
  }
#elif USING_CPP3K
  namespace meta = cpp3k::meta;
  template<typename T, typename Id, size_t I, size_t ...J>
  static constexpr bool equals_member(std::index_sequence<J...>&&) {
    return ((Id{}[hana::size_c<J>] ==
             cpp3k::meta::cget<I>($T.member_variables()).name()[J]) && ...);
  }

  template<typename T>
  struct get_matching_index {
    template<typename Id, std::size_t ...I>
    static constexpr std::size_t apply(Id&& id, std::index_sequence<I...>) {
      constexpr auto N = hana::length(Id{});
      return ((equals_member<T, Id, I>(
               std::make_index_sequence<N>{}) ? I : 0) + ...);
    }
  };

  template<typename T, typename Id>
  static constexpr auto get_metainfo_for(Id&& id) {
    constexpr auto N = $T.member_variables().size();
    return cpp3k::meta::cget<
        get_matching_index<T>::apply(Id{}, std::make_index_sequence<N>{})
      >($T.member_variables());
  }
#endif

  template<typename Id, typename Flag, typename ShortFlag, typename Help>
  struct Option
  {
    static constexpr Id identifier;
    static constexpr Flag flag;
    static constexpr ShortFlag short_flag;
    static constexpr Help help;
  };

  template<typename OptionsStruct>
  struct OptionsMap {
    static constexpr auto collect_flags = [](auto&& x, auto&& field) {
#if USING_REFLEXPR
      using T = UNWRAP_TYPE(field);
#elif USING_CPP3K
      using T = refl::unreflect_member_t<OptionsStruct, decltype(field)>;
#endif
      auto result = hana::insert(
          x, hana::make_pair(T::flag,
            get_metainfo_for<OptionsStruct>(T::identifier)));
      if constexpr(!hana::is_empty(T::short_flag)) {
        return hana::insert(
          result, hana::make_pair(T::short_flag,
            get_metainfo_for<OptionsStruct>(T::identifier)));
      } else {
        return result;
      }
    };

#if USING_REFLEXPR
    using MetaOptions = reflexpr(OptionsStruct);
    template<typename... MetaFields>
    struct make_prefix_map {
      static constexpr auto helper() {
        auto filtered = hana::filter(
          hana::make_tuple(hana::type_c<refl::unreflect_type<MetaFields>>...),
          [](auto&& field) {
            return hana::bool_c<
              metap::is_specialization<std::decay_t<UNWRAP_TYPE(field)>, Option>{}>;
          }
        );
        static_assert(!hana::length(filtered) == hana::size_c<0>,
            "No options found. Did you define options with the REFLOPT_OPTION macro?");
        return hana::fold(
          filtered,
          hana::make_map(),
          collect_flags
        );
      }
    };

    static constexpr auto prefix_map = meta::unpack_sequence_t<
      meta::get_data_members_m<MetaOptions>, make_prefix_map>::helper();

#elif USING_CPP3K
    static constexpr auto filtered = hana::filter(
        refl::adapt_to_hana($OptionsStruct.member_variables()),
        [](auto&& field) {
          using T = refl::unreflect_member_t<
            OptionsStruct, std::decay_t<decltype(field)>>;
          return hana::bool_c<metap::is_specialization<T, Option>{}>;
        }
      );
    static_assert(!hana::length(filtered) == hana::size_c<0>,
        "No options found. Did you define options with the REFLOPT_OPTION macro?");
    static constexpr auto prefix_map = hana::fold(
      filtered,
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

template<size_t N>
struct hana_string_from_literal {
  static constexpr auto apply(const char (&literal)[N]) {
    return apply_helper(literal, std::make_index_sequence<N>{});
  }

  template<size_t ...I>
  static constexpr auto apply_helper(const char (&literal)[N], std::index_sequence<I...>&&) {
    return hana::string_c<literal[I]...>;
  }
};

}  // namespace reflopt


#define BOOST_HANA_STRING_T(Literal) \
  decltype(Literal ## _s)

#define REFLOPT_OPTION_HELPER(Type, Identifier, Flag, ShortFlag, Help) \
  reflopt::Option<BOOST_HANA_STRING_T(#Identifier), BOOST_HANA_STRING_T(Flag), \
      BOOST_HANA_STRING_T(ShortFlag), BOOST_HANA_STRING_T(Help)> \
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define REFLOPT_OPTION_3(Type, Identifier, Flag) \
  REFLOPT_OPTION_HELPER(Type, Identifier, Flag, "", "")

#define REFLOPT_OPTION_4(Type, Identifier, Flag, ShortFlag) \
  REFLOPT_OPTION_HELPER(Type, Identifier, Flag, ShortFlag, "")

#define REFLOPT_OPTION_5(Type, Identifier, Flag, ShortFlag, Help) \
  REFLOPT_OPTION_HELPER(Type, Identifier, Flag, ShortFlag, Help)

#define REFLOPT_OPTION(...) \
  VRM_PP_CAT(REFLOPT_OPTION_, VRM_PP_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__) \
