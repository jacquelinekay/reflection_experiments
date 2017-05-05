#pragma once

#include <algorithm>
#include <string>
#include <string_view>

#include <boost/lexical_cast.hpp>

#include "macros.hpp"
#include "meta_utilities.hpp"
#include "refl_utilities.hpp"

#if USING_REFLEXPR
#include <reflexpr>
#elif USING_CPP3K
#else
NO_REFLECTION_IMPLEMENTATION_FOUND()
#endif

namespace reflser {

#if USING_REFLEXPR
namespace meta = std::meta;
#elif USING_CPP3K
namespace meta = cpp3k::meta;
#endif
namespace refl = jk::refl_utilities;
namespace metap = jk::metaprogramming;
namespace sl = jk::string_literal;

enum struct scan_result {
  continue_scanning,
  stop_scanning,
  error
};

// strip the surrounding whitespace
// TODO: other characters besides ' '
std::string_view strip_whitespace(const std::string_view& src) {
  unsigned i1 = 0;
  if (src[i1] == ' ') {
    for (; i1 < src.size() - 1; i1++) {
      if (src[i1] != ' ' || src[i1 + 1] != ' ') {
        i1 += 1;
        break;
      }
    }
  }

  unsigned i2 = src.size();
  if (src[i2 - 1] == ' ') {
    for (i2 = src.size() - 2; i2 > 0; i2--) {
      if (src[i2] != ' ') {
        i2 += 1;
        break;
      }
    }
  }

  return src.substr(i1, i2 - i1);
}

// returns the substring 
// needs to be able to propagate an error
template<typename T>
auto get_token_of_type(const std::string_view& src) {
  unsigned count = 0;

  auto condition = [](const std::string_view& str, unsigned i) {
    if constexpr (std::is_floating_point<T>{}) {
      char token = str[i];
      if (token == '-') {
        if (i != 0) {
          return scan_result::error;
        }
      }

      if (!std::isdigit(token) && token != '.') {
        return scan_result::stop_scanning;
      }
      return scan_result::continue_scanning;

    } else if constexpr (std::is_integral<T>{}) {
      char token = str[i];
      if (token == '-') {
        if constexpr (!std::is_signed<T>{}) {
          return scan_result::error;
        } else if (i != 0) {
          return scan_result::error;
        }
      }
      if (!std::isdigit(token)) {
        return scan_result::stop_scanning;
      }
      return scan_result::continue_scanning;
    } else {
      return scan_result::error;
    }
  };

  scan_result result;
  while ((result = condition(src, count++)) == scan_result::continue_scanning && count < src.size()) { }
  if (result == scan_result::error) {
    return std::string_view();
  }

  auto token = src.substr(0, count);

  if constexpr (std::is_floating_point<T>{}) {
    if (std::count(token.begin(), token.end(), '.') > 1) {
      return std::string_view();
    }
  }
  return token;
}

template<typename TokenT, typename T>
auto scan_for_end_token(TokenT open, TokenT close, const T& src) {
  // Scan src
  unsigned token_depth = 0;
  unsigned count = 0;
  do {
    if (src[count] == open) {
      ++token_depth;
    } else if (src[count] == close) {
      if (token_depth > 0) {
        --token_depth;
      } else {
        // we're done
        return count;
      }
    }
  } while (count++ < src.size());
  // better error indication?
  return count;
}

// Assumes that the first open token is already passed
template<typename TokenT, typename T>
auto count_outer_element_until_end(TokenT token,
    const std::string_view& open_tokens, const std::string_view& close_tokens,
    const T& src) {
  unsigned token_depth = 0;
  unsigned count = 0;
  unsigned token_count = 0;
  do {
    if (src[count] == token && token_depth == 0) {
      ++token_count;
    } else if (open_tokens.find(src[count]) != std::string::npos) {
      ++token_depth;
    } else if (close_tokens.find(src[count]) != std::string::npos) {
      if (token_depth > 0) {
        --token_depth;
      } else {
        return token_count;
      }
    }
  } while (count++ < src.size());
  // may want to indicate an error here
  return token_count;
}

template<typename TokenT, typename T>
auto scan_outer_element_until(TokenT token,
    const std::string_view& open_tokens, const std::string_view& close_tokens,
    const T& src) {
  // scan until token found
  unsigned token_depth = 0;
  unsigned count = 0;
  do {
    if (src[count] == token && token_depth == 0) {
      return src.substr(0, count);
    } else if (open_tokens.find(src[count]) != std::string::npos) {
      ++token_depth;
    } else if (close_tokens.find(src[count]) != std::string::npos) {
      if (token_depth > 0) {
        --token_depth;
      } else {
        return src.substr(0, count);
      }
    }
  } while (count++ < src.size());

  // better error indication?
  return std::string_view();
}

enum struct serialize_result {
  success,
  unknown_type
};

std::string serialize_result_message(serialize_result result) {
  switch(result) {
    case serialize_result::success:
      return "Success";
    case serialize_result::unknown_type:
      return "Don't know how to serialize to output type";
  }
}

// generic json serialization
template<typename T>
auto serialize(const T& src, std::string& dst) {
  if constexpr (std::is_same<T, std::string>{}) {
    dst += "\"" + src + "\"";
    return serialize_result::success;
  } else if constexpr (std::is_same<T, bool>{}) {
    dst += src ? "true" : "false";
    return serialize_result::success;
  } else if constexpr (metap::is_detected<metap::stringable, T>{}) {
    dst += std::to_string(src);
    return serialize_result::success;
  } else if constexpr (metap::is_detected<metap::iterable, T>{}) {
    // This structure has an array-like layout.
    dst += "[ ";
    for (auto it = src.begin(); it != src.end(); ++it) {
      auto entry = *it;
      auto result = serialize(entry, dst);
      if (result != serialize_result::success) {
        return result;
      }
      if (it != (src.end() - 1)) {
        dst += ", ";
      }
    }
    dst += " ]";
    return serialize_result::success;
#if USING_REFLEXPR
  } else if constexpr (meta::Record<reflexpr(T)>) {
#elif USING_CPP3K
  } else if constexpr (refl::is_member_type<T>()) {
#endif
    dst += "{ ";

    serialize_result result = serialize_result::success;

#if USING_REFLEXPR
    using MetaT = reflexpr(T);
    meta::for_each<meta::get_data_members_m<MetaT>>(
      [&src, &dst, &result](auto&& member_info){
        using MetaInfo = std::decay_t<decltype(member_info)>;
        dst += std::string("\"") + meta::get_base_name_v<MetaInfo> + "\"" + " : ";
        if (result = serialize(src.*meta::get_pointer<MetaInfo>::value, dst);
            result != serialize_result::success) {
          return;
        }
        dst += ", ";
      });
#elif USING_CPP3K
    meta::for_each($T.member_variables(),
      [&src, &dst, &result](auto&& member) {
        dst += std::string("\"") + member.name() + "\"" + " : ";
        if (result = serialize(src.*member.pointer(), dst);
            result != serialize_result::success) {
          return;
        }
        dst += ", ";
      }
    );
#endif
    // take off the last character
    if (result == serialize_result::success) {
      dst.replace(dst.size() - 2, 2, " }");
    }
    return result;
  }
  return serialize_result::unknown_type;
}

// generic json deserialization
enum struct deserialize_result {
  success,
  empty_input,
  malformed_input,
  mismatched_token,
  mismatched_type,
  unknown_type
};

std::string deserialize_result_message(deserialize_result result) {
  switch(result) {
    case deserialize_result::success:
      return "Success";
    case deserialize_result::empty_input:
      return "Input string to deserialize was empty";
    case deserialize_result::malformed_input:
      return "Input string to deserialize was malformed";
    case deserialize_result::mismatched_token:
      return "A token was mismatched (e.g. missing open or close brace)";
    case deserialize_result::mismatched_type:
      return "Type of output didn't match input schema (e.g. wrong number of fields)";
    case deserialize_result::unknown_type:
      return "Don't know how to deserialize to output type";
  }
}

// TODO: better error code
template<typename T>
auto deserialize(std::string_view& src, T& dst) {
  if (src.empty()) {
    return deserialize_result::empty_input;
  }
  if constexpr (std::is_same<T, std::string>{}) {
    // Scan until the first quote
    auto quote_index = std::find(src.begin(), src.end(), '"');

    if (quote_index == src.end()) {
      return deserialize_result::malformed_input;
    }
    src.remove_prefix(quote_index - src.begin() + 1);

    if (auto it = std::find(src.begin(), src.end(), '"'); it != src.end()) {
      auto index = it - src.begin();
      dst = src.substr(0, index);
      return deserialize_result::success;
    }
    return deserialize_result::malformed_input;
  } else if constexpr (std::is_same<T, bool>{}) {
    if (strip_whitespace(src).substr(0, 4) == "true") {
      dst = true;
      return deserialize_result::success;
    } else if (strip_whitespace(src).substr(0, 5) == "false") {
      dst = false;
      return deserialize_result::success;
    }
    return deserialize_result::malformed_input;
  } else if constexpr (std::is_arithmetic<T>{}) {
    auto token = get_token_of_type<T>(strip_whitespace(src));
    auto token_count = token.size();
    if (token_count == 0) {
      return deserialize_result::malformed_input;
    }

    dst = boost::lexical_cast<T>(token);
    return deserialize_result::success;
  } else if constexpr (metap::is_detected<metap::iterable, T>{}) {
    // TODO src is wrong
    // TODO strip_whitespace is too aggressive
    auto stripped = strip_whitespace(src);
    if (stripped[0] != '[') {
      return deserialize_result::malformed_input;
    }
    stripped.remove_prefix(1);
    auto array_end = scan_for_end_token('[', ']', stripped);
    if (array_end <= 1) {
      return deserialize_result::mismatched_token;
    }

    auto array_token = stripped.substr(0, array_end);

    auto n_elements = count_outer_element_until_end(',', "[{", "]}", array_token) + 1;

    if constexpr (metap::is_detected<metap::resizable, T>{}) {
      dst.resize(n_elements);
      // TODO case where the container has dynamic size and is not default-constructible
    } else if constexpr (metap::is_detected<metap::has_tuple_size, T>{}) {
      if (std::tuple_size<T>{} != n_elements) {
        return deserialize_result::mismatched_type;
      }
    }
    assert(n_elements == dst.size());

    for (unsigned index = 0; index < n_elements; index++) {
      auto token = scan_outer_element_until(',', "[{", "]}", array_token);
      array_token.remove_prefix(token.size());
      if (auto result = deserialize(token, dst[index]); result != deserialize_result::success) {
        return result;
      }
      if (array_token[0] == ',') {
        array_token.remove_prefix(1);
      }
    }

    src.remove_prefix(array_token.size());
    return deserialize_result::success;
#if USING_REFLEXPR
  } else if constexpr (meta::Record<reflexpr(T)>) {
    using MetaT = reflexpr(T);
#elif USING_CPP3K
  } else if constexpr (refl::is_member_type<T>()) {
#endif
    auto stripped = strip_whitespace(src);
    if (stripped[0] != '{') {
      return deserialize_result::malformed_input;
    }
    auto object_end = scan_for_end_token('{', '}', stripped);
    if (object_end == stripped.size()) {
      return deserialize_result::mismatched_token;
    }
    auto object_token = stripped.substr(1, object_end);

    auto n_colons = count_outer_element_until_end(':', "{[", "}]", object_token);
    auto n_commas = count_outer_element_until_end(',', "{[", "}]", object_token);

    if (n_colons != n_commas + 1) {
      return deserialize_result::malformed_input;
    }

    // TODO: switch for cpp3k
#if USING_REFLEXPR
    if (n_colons != meta::get_size<meta::get_data_members_m<MetaT>>{}) {
#elif USING_CPP3K
    if (n_colons != $T.member_variables().size()) {
#endif
      return deserialize_result::mismatched_type;
    }

    deserialize_result result = deserialize_result::success;
    for (unsigned i = 0; i < n_colons; ++i) {
      auto key_index = std::find(object_token.begin(), object_token.end(), ':') - object_token.begin();

      auto quote_index = std::find(object_token.begin(), object_token.begin() + key_index, '"') - object_token.begin();
      object_token.remove_prefix(quote_index + 1);
      quote_index = std::find(object_token.begin(), object_token.begin() + key_index, '"') - object_token.begin();

      const auto key = object_token.substr(0, quote_index);
      key_index = std::find(object_token.begin(), object_token.end(), ':') - object_token.begin();
      object_token.remove_prefix(key_index + 1);

      auto value_token = scan_outer_element_until(',', "{[", "}]", object_token);
      auto value_index = value_token.size();
      object_token.remove_prefix(value_index);

#if USING_REFLEXPR
      meta::for_each<meta::get_data_members_m<MetaT>>(
        [&dst, &key, &value_token, &result](auto&& metainfo) {
          using MetaInfo = std::decay_t<decltype(metainfo)>;
          constexpr auto name = meta::get_base_name_v<MetaInfo>;
          if (key == name) {
            constexpr auto p = refl::get_member_pointer<T>(sl::string_constant<name>{});
            if (result = deserialize(value_token, dst.*p);
                result != deserialize_result::success) {
              return;
            }
          }
        }
      );
#elif USING_CPP3K
      meta::for_each($T.member_variables(),
        [&dst, &key, &value_token, &result](auto&& member) {
          if (key == member.name()) {
            if (result = deserialize(value_token, dst.*member.pointer());
                result != deserialize_result::success) {
              return;
            }
          }
        }
      );
#endif
      if (result != deserialize_result::success) {
        return result;
      }
    }
    return deserialize_result::success;
  }
  return deserialize_result::unknown_type;
}

}  // namespace reflser
