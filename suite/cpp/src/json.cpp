// json: parse the orders into typed structs with nlohmann/json, sum two fields, write the document
// back
// (../SPEC.md section 5). ordered_json keeps the keys in document order; plain nlohmann::json sorts
// them.
#include <cstdint>
#include <format>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "inputs.hpp"
#include "lib.hpp"

namespace kernels {
namespace {

struct Order {
  std::uint64_t id;
  std::uint64_t customer_id;
  std::string status;
  std::uint64_t total_cents;
  std::uint64_t items;
  std::vector<std::string> tags;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Order, id, customer_id, status, total_cents, items, tags)

struct Document {
  std::vector<Order> orders;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Document, orders)

struct JsonResult {
  std::size_t orders;
  std::uint64_t total_cents;
  std::uint64_t items;
  std::string output;
};

JsonResult roundtrip(const std::string& text) {
  const auto doc = nlohmann::ordered_json::parse(text).get<Document>();
  std::uint64_t total_cents = 0;
  std::uint64_t items = 0;
  for (const Order& o : doc.orders) {
    total_cents += o.total_cents;
    items += o.items;
  }
  return {doc.orders.size(), total_cents, items, nlohmann::ordered_json(doc).dump()};
}

}  // namespace

Outcome json_kernel(const Args& args) {
  const auto text = read_file(args.data / std::format("orders-{}.json", args.size));
  auto [r, times] = measure(args.repeats, [&] { return roundtrip(text); });
  return {std::format("{}:{}:{}:{}:{}", r.orders, r.total_cents, r.items, r.output.size(),
                      fnv1a(r.output)),
          std::move(times),
          {}};
}

}  // namespace kernels
