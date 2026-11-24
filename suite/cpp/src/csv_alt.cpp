// csv-alt: the same with fast-cpp-csv-parser (io::CSVReader, two columns picked by header name).
#include <cstdint>
// GCC's -Wrestrict fires inside the library's error-message code after inlining, where -isystem
// does not reach.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
#include <fast-cpp-csv-parser/csv.h>
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include <format>
#include <string>

#include "csv_totals.hpp"
#include "work.hpp"

namespace kernels {
namespace {

Totals csv_totals(const std::string& path) {
  Totals t;
  io::CSVReader<2, io::trim_chars<' '>, io::double_quote_escape<',', '"'>> reader(path);
  reader.read_header(io::ignore_extra_column, "customer_id", "total_cents");
  std::uint32_t customer = 0;
  std::uint64_t total = 0;
  while (reader.read_row(customer, total)) {
    ++t.rows;
    t.per_customer[customer] += total;
  }
  return t;
}

}  // namespace

Outcome csv_alt_kernel(const Args& args) {
  const auto path = (args.data / std::format("orders-{}.csv", args.size)).string();
  auto [totals, times] = measure(args.repeats, [&] { return csv_totals(path); });
  return {csv_checksum(totals), std::move(times), {}};
}

}  // namespace kernels
