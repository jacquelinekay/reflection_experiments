/* Demo: Specify a struct for command line arguments
 * */

// TODO: pare down includes
#include <boost/hana.hpp>

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
  namespace hana = boost::hana;

  template<typename ...Ts>
  using tuple_t = std::tuple<Ts...>;

  template<typename T>
  using optional_t = std::experimental::optional<T>;

  template<typename KVPairs>
  constexpr auto map_fold(KVPairs&& kv_pairs) {
    constexpr auto pair_insert = [](auto&& m, auto&& element) {
      return m.insert(element[0], element[1]);
    };
    return boost::hana::fold(kv_pairs, boost::hana::make_map(), pair_insert);
  }

  template<typename OptionsStruct>
  struct OptionsMap {
    using MetaOptions = reflexpr(OptionsStruct);

    template<typename Indices, typename ...MetaFields>
    struct get_prefix_pairs_helper;

    template<size_t... Indices, typename ...MetaFields>
    struct get_prefix_pairs_helper<std::index_sequence<Indices...>, MetaFields...> {
      static constexpr auto filtered_fields = hana::filter(
          hana::make_tuple(hana::type_c<decl_data_member_type<Indices, MetaFields>::type>...),
          [](auto&& field){
            using T = typename std::decay_t<decltype(field)>::type;
            return has_member<T, decltype("identifier"_s)>::value
                && has_member<T, decltype("flag"_s)>::value;
          });
      static constexpr auto t = hana::fold(filtered_fields,
        [](auto&& x, auto&& field) {
          using T = decltype(field);
          auto result = x.append(hana::make_pair(field.flag, field.identifier));
          if constexpr(has_member<T, decltype("short_flag"_s)>::value) {
            result.append(hana::make_pair(field.short_flag, field.identifier));
          }
          return result;
        });
    };

    template<typename ...MetaFields>
    struct get_prefix_pairs : get_prefix_pairs_helper<std::index_sequence_for<MetaFields...>, MetaFields...> {
    };

    static constexpr auto prefix_map = map_fold(
      std::meta::unpack_sequence_t<std::meta::get_data_members_m<MetaOptions>, get_prefix_pairs>::t);


    static constexpr bool contains(const char* prefix) {
       // TODO
      return true;
    }

    static constexpr void set(OptionsStruct& options, const char* prefix, const char* value) {
      // retrieve the identifier name with the corresponding prefix and use that name to set the member pointer
      // need to cast value to the desired type
    }
  };

  // Basic implementation idea: accumulate a tuple 
  template<typename OptionsStruct>
  optional_t<OptionsStruct> parse(int argc, char** argv) {
    // Reflect on OptionsStruct
    // extract metadata from tag struct
    // map from tag with same name as identifier 
    // Set the member pointer in options using get_pointer

    // Need to construct runtime prefix-to-compile time index map
    //std::meta::get_element_m<MetaObj, i>

    OptionsStruct options;
    for (int i = 1; i < argc; i += 2) {
      /*
      if (OptionsMap<OptionsStruct>::contains(argv[i])) {
        OptionsMap<OptionsStruct>::set(options, argv[i], argv[i + 1]);
      } else {
        // unknown prefix found
        return std::experimental::nullopt;
      }
      */
    }

    return options;
  }

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
    /*
    static constexpr string_literal<Flag...> flag;
    static constexpr string_literal<ShortFlag...> short_flag;
    static constexpr string_literal<Id...> identifier;
    static constexpr string_literal<Help...> help;
    */
  };

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
  OPTION(std::string, filename, "--filenames");
  // default value declaration
  OPTION(int, iterations, "--iterations", "-i", "Number of times to run the algorithm.") = 100;
  OPTION(bool, help, "--help", "-h", "Print help and exit");
};

int main(int argc, char** argv) {
  auto args = reflopt::parse<ProgramOptions>(argc, argv);

  std::cout << args->filename << "\n";

  return 0;
}
