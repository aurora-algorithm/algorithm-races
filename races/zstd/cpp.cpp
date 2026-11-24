// Race: zstd (cpp)
// Extracted from suite/cpp/src/compress.cpp — build and run via suite/run.py

#include <zlib.h>
#include <zstd.h>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>
#include "inputs.hpp"
#include "lib.hpp"
#include "work.hpp"

namespace kernels {

using Bytes = std::vector<std::uint8_t>;

constexpr int kZstdLevel = 3;

Bytes read_bytes(const Args& args) {
  const auto text = read_file(args.data / std::format("corpus-{}.bin", args.size));
  return {text.begin(), text.end()};
}

Bytes zstd_compress(const Bytes& raw) {
  Bytes out(ZSTD_compressBound(raw.size()));
  const std::size_t n = ZSTD_compress(out.data(), out.size(), raw.data(), raw.size(), kZstdLevel);
  if (ZSTD_isError(n)) {
    throw std::runtime_error(ZSTD_getErrorName(n));
  }
  out.resize(n);
  return out;
}

Bytes zstd_decompress(const Bytes& packed) {
  const unsigned long long size = ZSTD_getFrameContentSize(packed.data(), packed.size());
  if (size == ZSTD_CONTENTSIZE_ERROR || size == ZSTD_CONTENTSIZE_UNKNOWN) {
    throw std::runtime_error("zstd frame without a content size");
  }
  Bytes out(static_cast<std::size_t>(size));
  const std::size_t n = ZSTD_decompress(out.data(), out.size(), packed.data(), packed.size());
  if (ZSTD_isError(n)) {
    throw std::runtime_error(ZSTD_getErrorName(n));
  }
  out.resize(n);
  return out;
}

std::string plain_checksum(const Bytes& raw) {
  return std::format("{}:{}", raw.size(), sha256_hex(raw));
}

Outcome zstd_kernel(const Args& args) {
  const auto raw = read_bytes(args);
  auto [packed, times] = measure(args.repeats, [&] { return zstd_compress(raw); });
  return {plain_checksum(zstd_decompress(packed)), std::move(times),
          std::format(R"({{"compressed_bytes":{}}})", packed.size())};
}

}  // namespace kernels
