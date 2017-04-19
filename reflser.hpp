#pragma once

#include <algorithm>
#include <string>
#include <reflexpr>

#include <boost/lexical_cast.hpp>

#include "refl_utilities.hpp"

#include <iostream>

namespace reflser {

namespace meta = std::meta;
namespace refl = jk::refl_utilities;

template<typename T>
using stringable = std::void_t<decltype(std::to_string(std::declval<T>()))>;

template<typename T>
using iterable = std::void_t<
  decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>;

template<typename T>
using resizable = std::void_t<decltype(std::declval<T>().resize())>;

template<typename T>
using has_tuple_size = std::void_t<std::tuple_size<T>>;

template<template<typename ...> typename Op, typename... Args>
using is_detected = std::experimental::is_detected<Op, Args...>;

enum struct scan_result {
  continue_scanning,
  stop_scanning,
  error
};

// strip the surrounding whitespace
std::string_view strip_whitespace(const std::string_view& src) {
  auto i1 = std::find(src.begin(), src.end(), ' ');
  auto i2 = std::find(i1 + 1, src.end(), ' ');
  return src.substr(i1 + 1 - src.begin(), i2 - i1);
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
auto count_outer_element_until_end(TokenT token, TokenT open, TokenT close, const T& src) {
  unsigned token_depth = 0;
  unsigned count = 0;
  unsigned token_count = 0;
  do {
    if (src[count] == token && token_depth == 0) {
      ++token_count;
    } else if (src[count] == open) {
      ++token_depth;
    } else if (src[count] == close) {
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
auto scan_outer_element_until(TokenT token, TokenT open, TokenT close, const T& src) {
  // scan until token found
  unsigned token_depth = 0;
  unsigned count = 0;
  do {
    if (src[count] == token && token_depth == 0) {
      return src.substr(0, count);
    } else if (src[count] == open) {
      ++token_depth;
    } else if (src[count] == close) {
      if (token_depth > 0) {
        --token_depth;
      } else {
        // return src.substr(0, count);
        return std::string_view();
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

// generic json serialization
template<typename T>
auto serialize(const T& src, std::string& dst) {
  using MetaT = reflexpr(T);
  if constexpr (std::is_same<T, std::string>{}) {
    dst += "\"" + src + "\"";
    return serialize_result::success;
  } else if constexpr (std::is_same<T, bool>{}) {
    dst += src ? "true" : "false";
    return serialize_result::success;
  } else if constexpr (is_detected<stringable, T>{}) {
    dst += std::to_string(src);
    return serialize_result::success;
  } else if constexpr (is_detected<iterable, T>{}) {
    // This structure has an array-like layout.
    dst += "[ ";
    for (const auto& entry : src) {
      dst += serialize(entry, dst);
      if (&entry != src.end()) {
        dst += ", ";
      }
    }
    dst + "] ";
    return serialize_result::success;
  } else if constexpr (meta::Record<MetaT>) {
    dst += "{ ";

    meta::for_each<meta::get_data_members_m<MetaT>>(
      [&src, &dst](auto&& member_info){
        using MetaInfo = std::decay_t<decltype(member_info)>;
        dst += std::string("\"") + meta::get_base_name_v<MetaInfo> + "\"" + " : ";
        if (serialize(src.*meta::get_pointer<MetaInfo>::value, dst) != serialize_result::success) {
          // TODO: error handling
          // return false;
        }
        dst += ", ";
      });
    // take off the last character
    dst.replace(dst.size() - 2, 2, " }");
    return serialize_result::success;
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
  using MetaT = reflexpr(T);
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
    auto stripped = strip_whitespace(src);
    auto token = get_token_of_type<T>(std::move(stripped));
    auto token_count = token.size();
    if (token_count == 0) {
      return deserialize_result::malformed_input;
    }

    dst = boost::lexical_cast<T>(token);
    return deserialize_result::success;
  } else if constexpr (is_detected<iterable, T>{}) {
    if (src[0] != '[') {
      return deserialize_result::malformed_input;
    }
    auto array_end = scan_for_end_token('[', ']', src);
    if (array_end == src.size()) {
      return deserialize_result::mismatched_token;
    }

    auto array_token = src.substr(1, array_end);

    auto n_elements = count_outer_element_until_end(',', '[', ']', array_token);

    if constexpr (is_detected<resizable, T>{}) {
      dst.resize(n_elements);
      // TODO case where the container has dynamic size and is not default-constructible
    } else if constexpr (is_detected<has_tuple_size, T>{}) {
      if (std::tuple_size<T>{} != n_elements) {
        return deserialize_result::mismatched_type;
      }
    }

    for (unsigned index = 0; index < n_elements; index++) {

      auto token = scan_outer_element_until(',', '[', ']', array_token);
      if (auto result = deserialize(token, dst[index]); result != deserialize_result::success) {
        return result;
      }
      // remove 1 for the comma (or ] if we're the last one)
      array_token.remove_prefix(token.size() + 1);
    }

    src.remove_prefix(array_token.size());
  } else if constexpr (meta::Record<MetaT>) {
    if (src[0] != '{') {
      return deserialize_result::malformed_input;
    }
    auto object_end = scan_for_end_token('{', '}', src);
    if (object_end == src.size()) {
      return deserialize_result::mismatched_token;
    }
    auto object_token = src.substr(1, object_end);

    auto n_colons = count_outer_element_until_end(':', '{', '}', object_token);
    auto n_commas = count_outer_element_until_end(',', '{', '}', object_token);

    if (n_colons != n_commas + 1) {
      return deserialize_result::malformed_input;
    }

    if (n_colons != meta::get_size<meta::get_data_members_m<MetaT>>{}) {
      return deserialize_result::mismatched_type;
    }

    for (unsigned i = 0; i < meta::get_size<meta::get_data_members_m<MetaT>>{}; ++i) {
      auto key_index = std::find(object_token.begin(), object_token.end(), ':') - object_token.begin();

      auto quote_index = std::find(object_token.begin(), object_token.begin() + key_index, '"') - object_token.begin();
      object_token.remove_prefix(quote_index + 1);
      quote_index = std::find(object_token.begin(), object_token.begin() + key_index, '"') - object_token.begin();

      auto key = object_token.substr(0, quote_index);
      key_index = std::find(object_token.begin(), object_token.end(), ':') - object_token.begin();
      object_token.remove_prefix(key_index + 1);

      auto value_index = std::find(object_token.begin(), object_token.end(), ',') - object_token.begin();
      auto value_token = object_token.substr(0, value_index);
      object_token.remove_prefix(value_index);

      // TODO propagate return value!
      meta::for_each<meta::get_data_members_m<MetaT>>(
        [&dst, &key, &value_token](auto&& metainfo) {
          using MetaInfo = std::decay_t<decltype(metainfo)>;
          constexpr auto name = meta::get_base_name_v<MetaInfo>;
          if (key == name) {
            constexpr auto p = refl::member_pointer<T, name>();
            deserialize(value_token, dst.*p);
          }
        }
      );

    }
    return deserialize_result::success;
  }
  return deserialize_result::unknown_type;
}

}  // namespace reflser
