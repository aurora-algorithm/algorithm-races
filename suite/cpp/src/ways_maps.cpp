// The builtins group's hash maps (std::unordered_map, ankerl::unordered_dense, Boost.Unordered)
// and string joins (std::string +=, reserve, std::ostringstream, std::views::join_with).
#include <ankerl/unordered_dense.h>

#include <boost/unordered/unordered_flat_map.hpp>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "inputs.hpp"
#include "kernels.hpp"
#include "ways.hpp"

namespace kernels {
namespace {

struct Counted {
  std::size_t len;
  std::size_t hits;
};

// `++m[k]` for every insert, then how many lookups are keys; the same code for every map type.
// The map is destroyed inside the timed call, as Rust drops its map there.
template <class Map, class K>
Counted count(const std::vector<K>& inserts, const std::vector<K>& lookups) {
  Map counts;
  for (const K& k : inserts) {
    ++counts[k];
  }
  std::size_t hits = 0;
  for (const K& k : lookups) {
    hits += counts.contains(k) ? 1 : 0;
  }
  return {counts.size(), hits};
}

template <class K>
Outcome count_with(const Args& args, std::string_view way, const std::vector<K>& inserts,
                   const std::vector<K>& lookups) {
  Counted (*run)(const std::vector<K>&, const std::vector<K>&) = nullptr;
  if (way == "std.unordered_map") {
    run = count<std::unordered_map<K, std::uint64_t>, K>;
  } else if (way == "ankerl.unordered_dense.map") {
    run = count<ankerl::unordered_dense::map<K, std::uint64_t>, K>;
  } else if (way == "boost.unordered_flat_map") {
    run = count<boost::unordered_flat_map<K, std::uint64_t>, K>;
  } else {
    throw std::invalid_argument(std::format("unknown map way {}", way));
  }
  auto [r, times] = measure(args.repeats, [&] { return run(inserts, lookups); });
  return {std::format("{}:{}", r.len, r.hits), std::move(times), {}};
}

std::string append(const std::vector<std::string>& parts) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out += ',';
    }
    out += parts[i];
  }
  return out;
}

// The total length is added up inside the timed call, as Rust's with_capacity way does.
std::string reserve(const std::vector<std::string>& parts) {
  std::size_t total = parts.empty() ? 0 : parts.size() - 1;
  for (const std::string& p : parts) {
    total += p.size();
  }
  std::string out;
  out.reserve(total);
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out += ',';
    }
    out += parts[i];
  }
  return out;
}

// str() copies the buffer into the returned string, as most code that builds a string this way
// does; that copy is timed.
std::string string_stream(const std::vector<std::string>& parts) {
  std::ostringstream out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      out << ',';
    }
    out << parts[i];
  }
  return out.str();
}

std::string join_with(const std::vector<std::string>& parts) {
  return std::ranges::to<std::string>(parts | std::views::join_with(','));
}

}  // namespace

namespace ways {

Outcome map_ints(const Args& args, std::string_view way) {
  const auto [inserts, lookups] = hashmap_input(args.size);
  return count_with(args, way, inserts, lookups);
}

// Keys are string_views into the input strings, as the other languages store references to the
// existing strings; no key is copied.
Outcome map_strings(const Args& args, std::string_view way) {
  const std::vector<std::string> owned = strings(args.size);
  const std::vector<std::string_view> inserts(owned.begin(), owned.end());
  std::vector<std::string_view> lookups(inserts);
  for (std::size_t i = 1; i < lookups.size(); i += 2) {
    lookups[i].remove_suffix(1);
  }
  return count_with(args, way, inserts, lookups);
}

Outcome join(const Args& args, std::string_view way) {
  std::string (*build)(const std::vector<std::string>&) = nullptr;
  if (way == "std.string.append") {
    build = append;
  } else if (way == "std.string.reserve") {
    build = reserve;
  } else if (way == "std.ostringstream") {
    build = string_stream;
  } else if (way == "std.views.join_with") {
    build = join_with;
  } else {
    throw std::invalid_argument(std::format("unknown join way {}", way));
  }
  std::vector<std::string> parts;
  parts.reserve(args.size);
  for (const std::int64_t v : ints(args.size)) {
    parts.push_back(std::to_string(v));
  }
  auto [joined, times] = measure(args.repeats, [&] { return build(parts); });
  return {std::format("{}:{}", joined.size(), fnv1a(joined)), std::move(times), {}};
}

}  // namespace ways
}  // namespace kernels
