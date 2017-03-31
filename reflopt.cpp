/* Demo: Specify a struct for command line arguments
 * */

// TODO: pare down includes
#include <boost/hana.hpp>

#include <boost/lexical_cast.hpp>

#include <reflexpr>
#include <experimental/optional>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>

#include "refl_utilities.hpp"
#include "string_literal.hpp"

#include <vrm/pp/arg_count.hpp>
#include <vrm/pp/cat.hpp>


namespace reflopt {
  static const size_t max_value_length = 128;

  namespace hana = boost::hana;

  template<typename T>
  using optional_t = std::experimental::optional<T>;

  template<typename T, typename IdentifierName, typename FlagName, typename ShortFlag, typename HelpString>
  struct Option;

  template<typename T, char... Id, char... Flag, char... ShortFlag, char... Help>
  struct Option<T, string_literal<Id...>, string_literal<Flag...>,
                   string_literal<ShortFlag...>, string_literal<Help...>>
  {
    static constexpr char flag[sizeof...(Flag)] = pack_to_literal<string_literal<Flag...>>();
    static constexpr char short_flag[sizeof...(ShortFlag)] = pack_to_literal<string_literal<ShortFlag...>>();
    static constexpr char identifier[sizeof...(Id)] = pack_to_literal<string_literal<Id...>>();
    static constexpr char help[sizeof...(Help)] = pack_to_literal<string_literal<Help...>>();

    using flag_t = string_literal<Flag...>;
    using identifier_t = string_literal<Id...>;
    using short_flag_t = string_literal<ShortFlag...>;
    using help_t = string_literal<Help...>;
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
            using MetaT = typename std::decay_t<decltype(field)>::type;
            using T = std::meta::get_reflected_type_t<std::meta::get_type_m<MetaT>>;
            return hana::bool_c<is_specialization<T, Option>{}>;
          }
        );
        return hana::fold(
          // hana::make_tuple(hana::type_c<MetaFields>...),
          filtered,
          hana::make_map(),
          [](auto&& x, auto&& field) {
            using MetaT = typename std::decay_t<decltype(field)>::type;
            using T = std::meta::get_reflected_type_t<std::meta::get_type_m<MetaT>>;
            return hana::insert(
                x, hana::make_pair(hana::type_c<typename T::flag_t>, hana::type_c<typename T::identifier_t>));
            /*
             // TODO: Fix meta_has_member
            if constexpr(meta_has_member<MetaT, decltype("short_flag"_s)>::value) {
              return hana::insert(result, hana::make_pair(hana::type_c<T::short_flag_t>, hana::type_c<T::identifier_t>));
            } else {
              return result;
            }
            */
          });
      }
    };

    static constexpr auto prefix_map =
      std::meta::unpack_sequence_t<std::meta::get_data_members_m<MetaOptions>, get_prefix_pairs>::helper();


    static bool contains(const char* prefix) {
      return hana::fold(hana::keys(prefix_map),
        false,
        [&prefix](bool x, auto&& key) {
          return x || compare<typename std::decay_t<decltype(key)>::type>(prefix);
        }
      );
    }

    static auto set(OptionsStruct& options, const char* prefix, const char* value) {
      // Match prefix with a compile-time key!
      hana::for_each(hana::keys(prefix_map),
        [&options, &prefix, &value](auto&& key) {
          using Key = typename std::decay_t<decltype(key)>::type;
          if (compare<typename std::decay_t<decltype(key)>::type>(prefix)) {
            // retrieve the identifier name with the corresponding prefix
            // use that name to set the member pointer
            auto id = hana::at_key(prefix_map, key);
            using ID = typename decltype(id)::type;

            constexpr auto member_pointer = get_member_pointer<OptionsStruct, ID>::value;
            options.*member_pointer = boost::lexical_cast<typename get_member_type<OptionsStruct, ID>::type>(
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
        std::cerr << "Unknown prefix found: " << argv[i] << "\n";
        std::cerr << "map keys:\n";
        hana::for_each(hana::keys(OptionsMap<OptionsStruct>::prefix_map),
          [](auto&& k) {
            std::cerr << literal_type_to_string<typename std::decay_t<decltype(k)>::type>::convert() << "\n";
          }
        );

        return std::experimental::nullopt;
      }
    }

    return options;
  }

}  // namespace reflopt

#define OPTION_3(Type, Identifier, Flag) \
  reflopt::Option<Type, decltype(#Identifier ## _s), decltype(Flag ## _s), string_literal<>, string_literal<>> \
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define OPTION_4(Type, Identifier, Flag, ShortFlag) \
  reflopt::Option<Type, decltype(#Identifier ## _s), decltype(Flag ## _s), decltype(ShortFlag ## _s), string_literal<>>\
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define OPTION_5(Type, Identifier, Flag, ShortFlag, Help) \
  reflopt::Option<Type, decltype(#Identifier ## _s), decltype(Flag ## _s), decltype(ShortFlag ## _s), decltype(Help ## _s)>\
    reflopt_ ## Identifier ## _tag; \
  Type Identifier

#define OPTION(...) VRM_PP_CAT(OPTION_, VRM_PP_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__) \


struct ProgramOptions {
  OPTION(std::string, filename, "--filename");
  // default value declaration
  OPTION(int, iterations, "--iterations", "-i", "Number of times to run the algorithm.") = 100;
  OPTION(bool, help, "--help", "-h", "Print help and exit");
};

int main(int argc, char** argv) {
  auto args = reflopt::parse<ProgramOptions>(argc, argv);
  if (!args) {
    std::cerr << "Argument parsing failed." << std::endl;
    // TODO: Automated help string
    return -1;
  }

  std::cout << args->filename << "\n";
  std::cout << args->iterations << "\n";

  return 0;
}
