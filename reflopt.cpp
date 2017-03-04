/* Demo: Specify a struct for command line arguments
 * */

#include <reflexpr>
#include <experimental/optional>
#include <string>
#include <tuple>
#include <vector>

#include "refl_utilities.hpp"
#include "string_literal.hpp"

template<typename ...Types>
struct reduce_pack {
  // Find the type in the pack which is non-void

  template <typename Tn, void* ...v>
  static Tn f(decltype(v)..., Tn, ...);

  using type = decltype(f(std::declval<Types>()...));
};

namespace reflopt {
  template<typename ...Ts>
  using tuple_t = std::tuple<Ts...>;

  template<typename T>
  using optional_t = std::experimental::optional<T>;

  using nullopt_t = std::experimental::nullopt;

  struct flags {
    template<typename ... Flags>
    constexpr flags(Flags&&...) noexcept {
      // TODO
    }
  };

  struct help {
    constexpr help(const char* s) noexcept : help_string(s) {}
    const char* help_string;
  };

  // allowed initializers: +, ?, *, N
  struct nargs {
    constexpr nargs(const char* tag) {
      // TODO
    }

    constexpr nargs(std::size_t n) {
      // TODO
    }
  };


  // but we also want to get the field that matches 
  /*
  template<char...Name, typename ...MetaFields>
  struct has_field {
    constexpr static bool value = string_ops<Name...>::equal(std::meta::get_base_name_v<MetaFields>) || ...;
  };
  */

  template<typename OptionsStruct, typename MemberName, typename ...Args>
  struct Argument {
    using MetaObj = reflexpr(OptionsStruct);

    template<typename ...MetaFields>
    struct n_fields_helper {
      constexpr static int value = sizeof...(MetaFields);
    };

    constexpr static std::size_t n_fields = std::meta::unpack_sequence_t<
      std::meta::get_data_members_m<MetaObj>, n_fields_helper>::value;

    // Check that OptionsStruct contains a field equal to MemberLiteral
    /*
    static_assert(
        std::meta::unpack_sequence_t<std::meta::get_data_members_m<MetaObj>, has_field>::value,
        "Can't add argument for field that doesn't exist in options struct.");
        */

    template<typename ...MetaField>
    struct get_member {
      // using field = std::meta::get_base_name_v<MetaField>;
      using field = typename reduce_pack<
        typename std::conditional<
          unpack_string_literal<MemberName>::equal(std::meta::get_base_name_v<MetaField>),
            MetaField, void*
        >::type...
      >::type;
    };
    // get the data member with the requested name
    using MetaField = typename std::meta::unpack_sequence_t<std::meta::get_data_members_m<MetaObj>, get_member>::field;

    // using argument_type = std::meta::get_type_m<MetaField>;

    template<std::size_t ...i>
    constexpr static std::size_t index_helper(std::index_sequence<i...>) {
      return ((unpack_string_literal<MemberName>::equal(
            std::meta::get_base_name_v<std::meta::get_element_m<MetaObj, i>>) ? i : 0) + ...);
    }

    // we need a pack
    constexpr static std::size_t index = index_helper(std::make_index_sequence<n_fields>{});

    // Associate the member with the field somehow?
    // I guess we're going to order by index and then call an aggregate constructor

    // Argument parsing... oh boi
    // TODO Check for duplicates, check type validity
    Argument(Args&&... args) noexcept : options(std::make_tuple(args...)) {
    }

    tuple_t<Args...> options;
  };

  template<typename OptionsStruct, typename MemberName, typename ...Args>
  constexpr static auto arg(MemberName&&, Args&&... args) {
    return Argument<OptionsStruct, MemberName, Args...>(std::forward<Args>(args)...);
  }

  template<typename OptionsStruct, typename ...Arguments>
  struct Parser {
    using MetaObj = reflexpr(OptionsStruct);
    using OptionsTuple = std::meta::unpack_sequence_t<
      std::meta::get_data_members_m<MetaObj>, member_pack_as_tuple>;

    Parser(Arguments&&... args) noexcept : arguments(std::make_tuple(args...)) {
    }

    // Basic implementation idea: accumulate a tuple 
    optional_t<OptionsStruct> parse(int argc, char** argv) {
      OptionsTuple values;
      // Parse argc, argv into OptionsStruct
      if (argc == 1) {
        // Invalid
        if (sizeof...(Arguments) > 0) {
          return nullopt_t;
        }
        // Else, default-construct the struct
        return OptionsStruct{};
      }
      unsigned positional_argument_i = 0;

      // now, to begin parsing
      // Tokenize argv?
      // scan through tokens, if it matches a prefix argument, stuff it into the struct
      for (unsigned i = 0; i < argc; ++i) {
        if (prefix_map.contains(argv[i])) {
          if (i == argc - 1) {
            return nullopt_t;
          }

          add_argument(values, prefix_map[argv[i]], argv[++i]);
        }
        // otherwise, positional argument.
        add_argument(values, positional_argument_i++, argv[i]);
      }

      return std::make_from_tuple<OptionsStruct>(values);
    }

    tuple_t<Arguments...> arguments;
  };

  template<typename OptionsStruct, typename ...Arguments>
  auto register_args(Arguments&&... args) {
    // TODO
    return Parser<OptionsStruct, Arguments...>(std::forward<Arguments>(args)...);
  }
}  // namespace reflopt

struct ProgramOptions {
  std::vector<std::string> filenames;
  int iterations = 100;
  bool help;
};

int main(int argc, char** argv) {
  using namespace reflopt;

#if 0
  auto x = arg<ProgramOptions>(&ProgramOptions::iterations,
        , flags("-i")
        , help("How many times to run the algorithm.")
        , nargs("?"));


  // Here we annotate the fields of ProgramOptions
  // TODO: Eliminate redundancy of Programoptions tag
  auto parser = register_args<ProgramOptions>(
    arg<ProgramOptions>(
        "iterations"_s
        , flags("-i")
        , help("How many times to run the algorithm.")
        , nargs("?"))
    , arg<ProgramOptions>(
        "filenames"_s
        , help("The filename(s) to run the algorithm over.")
        , nargs("+")
      )
  );

  auto options = parser.parse(arg, argv);
  if (!options || options.help) {
    std::cout << parser.help() << "\n";
    return 1;
  }
#endif

}
