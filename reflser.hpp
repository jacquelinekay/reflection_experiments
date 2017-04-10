#pragma once

#include <reflexpr>

// TODO: separate into another head
#include <iostream>

#include "refl_utilities.hpp"

namespace reflser {

namespace meta = std::meta;
namespace refl = jk::refl_utilities;

// Concepts
/* Define serialization visitor:
 * - an overload set defining how individual types are serialized
 * - contains: internal state that gets modified based on the serialization rules
 * - what's the return type?
 * - how to indicate error?
 *   - have a type to indicate error, and provide an overload for handling that error as an input arg
 * */

/*
template<typename T>
constexpr const bool SerializationVisitor;

template<typename T>
constexpr const bool DeserializationVisitor;
*/

// Overloads are a bunch of function objects (probably lambdas) operating on primitive types
// no generic lambdas in the overload set!
// TODO: enforce type signatures for serialization and deserialization (ref vs const ref)
template<typename SerState, typename ...Overloads>
static constexpr auto make_recursive_visitor(Overloads&&... overloads) {
  auto overloaded_fns = refl::overload(overloads...);
  // TODO! perfect capture by move
  return y_combinator([&overloaded_fns](auto self, auto&& x, SerState state) {
    using T = decltype(x);
    constexpr bool callable = std::is_callable<decltype(overloaded_fns)(T, SerState)>{};
    constexpr bool has_data_members = refl::n_fields<T>{} > 0;
    static_assert(callable || has_data_members, "Unknown scalar type found, cannot serialize.");
    if constexpr (callable) {
      return overloaded_fns(x, state);
    } else if constexpr (has_data_members) {
      return fold_over_data_members(x, [&self, &state](auto&& y) { return self(y, state); });
    }
  });
}

// archive, prologue, epilogue
// define some specializations for a particular serialization format
//

// TODO: Return types.
//
template<typename Archive, typename Policy>
static constexpr auto archive_skeleton() {
  return y_combinator([](auto self, Archive& archive, auto& src) {
    using T = std::decay_t<decltype(src)>;
    using MetaT = reflexpr(T);

    // TODO: Containers.
    if constexpr (meta::Record<MetaT>) {
      // this will open/close the record scope. e.g. for json it provides open/close brackets
      auto s = Policy::record_scope(archive, src);

      // TODO commas though
      auto ret = meta_fold_over_data_members(src, [&self, &archive](auto&& meta_info, auto&& y) {
        if constexpr (refl::index_of_metainfo<T, decltype(meta_info)>{} != 0) {
          Policy::member_delimiter(archive);
        }
        Policy::emit_metainfo(archive, meta_info, y);
        self(archive, y);
      });
    } else if constexpr (refl::is_iterable<T>{}) {
      // TODO Special policy rules?
      // use iterators
      auto s = Policy::list_scope(archive, src);
      for (const auto& x : src) {
        self(archive, x);
        if (&x != src.end()) {
          Policy::list_delimiter(archive);
        }
      }
    } else {
      archive(src);
    };
  });
};

struct json_policy {
  // TODO: reverse by switching the operator? omfg
  // what about whitespace
  template<typename Archive, typename Record>
  struct record_scope {
    // encapsulates prologue/epilogue.
    record_scope(Archive& a, const Record&) : archive(a) {
      archive("{\n");
    }

    ~record_scope() {
      archive("}\n");
    }

    Archive& archive;
  };

  template<typename Archive, typename Record>
  struct list_scope {
    // encapsulates prologue/epilogue.
    list_scope(Archive& a, const Record&) : archive(a) {
      archive("[\n");
    }

    ~list_scope() {
      archive("]\n");
    }

    Archive& archive;
  };

  template<typename Archive, typename MetaInfo, typename T>
  static auto emit_metainfo(Archive& archive, MetaInfo&&, const T& t) {
    archive(meta::get_base_name<MetaInfo>{}, ": ");
  }

  template<typename Archive>
  static auto list_delimiter(Archive& archive) { archive(","); }

  template<typename Archive>
  static auto member_delimiter(Archive& archive) { archive(","); }

};

struct OstreamArchive {
  template<typename ...Args>
  auto operator()(const Args&... args) {
    (stream << ... << args);
  };

  std::ostream stream;
};

struct IstreamArchive {
  template<typename ...Args>
  auto operator()(Args&... args) {
    (stream >> ... >> args);
  };

  std::istream stream;
};

}  // namespace reflser
