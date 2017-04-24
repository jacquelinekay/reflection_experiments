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

#include <vrm/pp/arg_count.hpp>
#include <vrm/pp/cat.hpp>

// XXX
#include <iostream>
#include <boost/hana/length.hpp>

namespace reflopt {
  static const size_t max_value_length = 128;

  namespace refl = jk::refl_utilities;
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
    using MetaOptions = reflexpr(OptionsStruct);

    template<typename... MetaFields>
    struct get_prefix_pairs {
      static constexpr auto helper() {
        auto filtered = hana::filter(
          hana::make_tuple(hana::type_c<refl::unreflect_type<MetaFields>>...),
          [](auto&& field) {
            return hana::bool_c<
              refl::is_specialization<std::decay_t<UNWRAP_TYPE(field)>, Option>{}>;
          }
        );
        static_assert(!hana::length(filtered) == hana::size_c<0>);
        return hana::fold(
          filtered,
          hana::make_map(),
          [](auto&& x, auto&& field) {
            // using T = refl::unreflect_type<UNWRAP_TYPE(field)>;
            using T = UNWRAP_TYPE(field);
            auto result = hana::insert(
                x, hana::make_pair(hana::type_c<decltype(T::flag)>, T::identifier));
            if constexpr(refl::has_member<T>(STRING_LITERAL("short_flag"))
                         && !sl::empty(T::short_flag)) {
              return hana::insert(
                result, hana::make_pair(hana::type_c<decltype(T::short_flag)>, T::identifier));
            } else {
              return result;
            }
          });
      }
    };

    static constexpr auto prefix_map =
      std::meta::unpack_sequence_t<
        std::meta::get_data_members_m<MetaOptions>, get_prefix_pairs>::helper();
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
            constexpr auto member_pointer = refl::get_member_pointer<OptionsStruct>(id);

            using MemberType = typename refl::get_member_type<OptionsStruct, decltype(id)>::type;
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
