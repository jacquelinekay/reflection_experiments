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

namespace reflopt {
  static const size_t max_value_length = 128;

  namespace refl = jk::refl_utilities;
  namespace sl = jk::string_literal;
  namespace hana = boost::hana;

  template<typename T>
  using optional_t = std::experimental::optional<T>;

  template<typename T, typename IdentifierName, typename FlagName, typename ShortFlag, typename HelpString>
  struct Option;

  template<typename T, char... Id, char... Flag, char... ShortFlag, char... Help>
  struct Option<T, sl::string_literal<Id...>, sl::string_literal<Flag...>,
                   sl::string_literal<ShortFlag...>, sl::string_literal<Help...>>
  {
    using flag_t = sl::string_literal<Flag...>;
    using identifier_t = sl::string_literal<Id...>;
    using short_flag_t = sl::string_literal<ShortFlag...>;
    using help_t = sl::string_literal<Help...>;
  };

  template<typename OptionsStruct>
  struct OptionsMap {
    using MetaOptions = reflexpr(OptionsStruct);

    template<typename... MetaFields>
    struct get_prefix_pairs {
      static constexpr auto helper() {
        auto filtered = hana::filter(
          hana::make_tuple(hana::type_c<MetaFields>...),
          [](auto&& field) {
            return hana::bool_c<
              refl::is_specialization<refl::unreflect_type<UNWRAP_TYPE(field)>, Option>{}>;
          }
        );
        return hana::fold(
          filtered,
          hana::make_map(),
          [](auto&& x, auto&& field) {
            using T = refl::unreflect_type<UNWRAP_TYPE(field)>;
            auto result = hana::insert(
                x, hana::make_pair(
                  hana::type_c<typename T::flag_t>, hana::type_c<typename T::identifier_t>));
            if constexpr(refl::has_member<T, decltype("short_flag"_s)>::value
                         && !sl::empty<typename T::short_flag_t>{}) {
              return hana::insert(
                result, hana::make_pair(
                  hana::type_c<typename T::short_flag_t>, hana::type_c<typename T::identifier_t>));
            } else {
              return result;
            }
          });
      }
    };

    static constexpr auto prefix_map =
      std::meta::unpack_sequence_t<
        std::meta::get_data_members_m<MetaOptions>, get_prefix_pairs>::helper();


    static bool contains(const char* prefix) {
      return hana::fold(hana::keys(prefix_map),
        false,
        [&prefix](bool x, auto&& key) {
          return x || sl::compare<UNWRAP_TYPE(key)>(prefix);
        }
      );
    }

    static auto set(OptionsStruct& options, const char* prefix, const char* value) {
      hana::for_each(hana::keys(prefix_map),
        [&options, &prefix, &value](auto&& key) {
          if (sl::compare<UNWRAP_TYPE(key)>(prefix)) {
            using ID = UNWRAP_TYPE(hana::at_key(prefix_map, key));

            constexpr auto member_pointer = refl::get_member_pointer<OptionsStruct, ID>::value;
            using MemberType = typename refl::get_member_type<OptionsStruct, ID>::type;
            options.*member_pointer = boost::lexical_cast<MemberType>(
              value, strnlen(value, max_value_length));
          }
        }
      );
    }
  };

  template<typename OptionsStruct>
  optional_t<OptionsStruct> parse(int argc, char** argv) {
    // TODO error handling
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
  reflopt::Option<Type, decltype(#Identifier ## _s), decltype(Flag ## _s), \
  reflopt::sl::string_literal<>, reflopt::sl::string_literal<>> \
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define REFLOPT_OPTION_4(Type, Identifier, Flag, ShortFlag) \
  reflopt::Option<Type, decltype(#Identifier ## _s), decltype(Flag ## _s), \
  decltype(ShortFlag ## _s), reflopt::sl::string_literal<>>\
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define REFLOPT_OPTION_5(Type, Identifier, Flag, ShortFlag, Help) \
  reflopt::Option<Type, decltype(#Identifier ## _s), decltype(Flag ## _s), \
  decltype(ShortFlag ## _s), decltype(Help ## _s)>\
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define REFLOPT_OPTION(...) VRM_PP_CAT(REFLOPT_OPTION_, VRM_PP_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__) \

