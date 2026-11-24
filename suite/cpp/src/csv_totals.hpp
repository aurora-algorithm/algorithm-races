// The csv checksum shared by csv.cpp and csv_alt.cpp (../SPEC.md section 11).
#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <unordered_map>

namespace kernels {

struct Totals {
  std::uint64_t rows = 0;
  std::unordered_map<std::uint32_t, std::uint64_t> per_customer;
};

// "{rows}:{customers}:{sum}:{top id}:{top sum}", top = largest sum, ties to the smaller id.
inline std::string csv_checksum(const Totals& t) {
  std::uint64_t sum = 0;
  std::uint32_t top_id = 0;
  std::uint64_t top = 0;
  for (const auto& [id, total] : t.per_customer) {
    sum += total;
    if (total > top || (total == top && id < top_id)) {
      top_id = id;
      top = total;
    }
  }
  return std::format("{}:{}:{}:{}:{}", t.rows, t.per_customer.size(), sum, top_id, top);
}

}  // namespace kernels
