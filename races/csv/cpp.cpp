// Race: csv (cpp)
// Extracted from suite/cpp/src/csv.cpp — build and run via suite/run.py

// csv: parse the orders CSV with vincentlaucsb/csv-parser (columns by header name), sum totals per
// customer.
#include <csv-parser/csv.hpp>
#include <format>
#include <string>

#include "csv_totals.hpp"
#include "work.hpp"

namespace kernels {
namespace {

Totals csv_totals(const std::string& path) {
  Totals t;
  csv::CSVReader reader(path);
  for (csv::CSVRow& row : reader) {
    ++t.rows;
    t.per_customer[row["customer_id"].get<std::uint32_t>()] +=
        row["total_cents"].get<std::uint64_t>();
  }
  return t;
}

}  // namespace

Outcome csv_kernel(const Args& args) {
  const auto path = (args.data / std::format("orders-{}.csv", args.size)).string();
  auto [totals, times] = measure(args.repeats, [&] { return csv_totals(path); });
  return {csv_checksum(totals), std::move(times), {}};
}

}  // namespace kernels
