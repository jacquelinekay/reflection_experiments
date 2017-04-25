#pragma once

#include <boost/hana/at_key.hpp>
#include <boost/hana/filter.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/tuple.hpp>

#include <boost/lexical_cast.hpp>

#include <experimental/optional>

#include "string_literal.hpp"
#include "refl_utilities.hpp"
#include "meta_utilities.hpp"

#include <vrm/pp/arg_count.hpp>
#include <vrm/pp/cat.hpp>

// XXX
#include <iostream>
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
  namespace sl = jk::string_literal;
  namespace hana = boost::hana;

  template<typename T>
  using optional_t = std::experimental::optional<T>;

  template<typename T, typename Id, typename Flag, typename ShortFlag, typename Help>
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
          x, hana::make_pair(
            hana::type_c<decltype(T::flag)>, refl::get_metainfo_for<OptionsStruct>(T::identifier)));
      if constexpr(!sl::empty(T::short_flag)) {
        return hana::insert(
          result, hana::make_pair(
            hana::type_c<decltype(T::short_flag)>, refl::get_metainfo_for<OptionsStruct>(T::identifier)));
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

    static constexpr auto prefix_map =
      std::meta::unpack_sequence_t<
        std::meta::get_data_members_m<MetaOptions>, make_prefix_map>::helper();

#elif USING_CPP3K

    static constexpr auto filtered = hana::filter(
        refl::adapt_to_hana($OptionsStruct.member_variables()),
        [](auto&& field) {
          using T = refl::unreflect_member_t<OptionsStruct, decltype(field)>;
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
          using T = UNWRAP_TYPE(key);
          return x || sl::equal(UNWRAP_TYPE(key){}, prefix);
        }
      );
    }

    static auto set(OptionsStruct& options, const char* prefix, const char* value) {
      hana::for_each(hana::keys(prefix_map),
        [&options, &prefix, &value](auto&& key) {
          using T = UNWRAP_TYPE(key);
          if (sl::equal(T{}, prefix)) {
            constexpr auto id = hana::at_key(prefix_map, hana::type_c<T>);

            // should the map just store the member info
#if USING_REFLEXPR
            using MetaInfo = std::decay_t<decltype(id)>;
            constexpr auto member_pointer = std::meta::get_pointer<MetaInfo>::value;
            using MemberType = std::meta::get_reflected_type_t<std::meta::get_type_m<MetaInfo>>;
#elif USING_CPP3K
            constexpr auto member_pointer = id.pointer();
            using MemberType = refl::unreflect_member_t<OptionsStruct, decltype(id)>;
#endif
            options.*member_pointer = boost::lexical_cast<MemberType>(
              value, strnlen(value, max_value_length));
          }
        }
      );
    }
  };

  template<typename OptionsStruct>
  optional_t<OptionsStruct> parse(int argc, char** argv) {
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

#define REFLOPT_OPTION_3(Type, Identifier, Flag) \
  STRING_TYPE_DECL(Identifier ## _string_t, #Identifier) \
  STRING_TYPE_DECL(Identifier ## _flag_string_t, Flag) \
  reflopt::Option<Type, Identifier ## _string_t, Identifier ## _flag_string_t, \
    jk::string_literal::empty_string_t, jk::string_literal::empty_string_t> \
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define REFLOPT_OPTION_4(Type, Identifier, Flag, ShortFlag) \
  STRING_TYPE_DECL(Identifier ## _string_t, #Identifier) \
  STRING_TYPE_DECL(Identifier ## _flag_string_t, Flag) \
  STRING_TYPE_DECL(Identifier ## _short_flag_string_t, ShortFlag) \
  reflopt::Option<Type, Identifier ## _string_t, Identifier ## _flag_string_t, \
    Identifier ## _short_flag_string_t, jk::string_literal::empty_string_t> \
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define REFLOPT_OPTION_5(Type, Identifier, Flag, ShortFlag, Help) \
  STRING_TYPE_DECL(Identifier ## _string_t, #Identifier) \
  STRING_TYPE_DECL(Identifier ## _flag_string_t, Flag) \
  STRING_TYPE_DECL(Identifier ## _short_flag_string_t, ShortFlag) \
  STRING_TYPE_DECL(Identifier ## _help_string_t, Help) \
  reflopt::Option<Type, Identifier ## _string_t, Identifier ## _flag_string_t, \
    Identifier ## _short_flag_string_t, Identifier ## _help_string_t> \
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define REFLOPT_OPTION(...) VRM_PP_CAT(REFLOPT_OPTION_, VRM_PP_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__) \
