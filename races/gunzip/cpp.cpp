// Race: gunzip (cpp)
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

constexpr int kGzipLevel = 6;

constexpr int kGzipWindow = 15 + 16;  // 2^15 window, gzip header and trailer

constexpr std::size_t kInflateChunk = std::size_t{1} << 16;

Bytes read_bytes(const Args& args) {
  const auto text = read_file(args.data / std::format("corpus-{}.bin", args.size));
  return {text.begin(), text.end()};
}

Bytes gzip(const Bytes& raw) {
  z_stream zs{};
  if (deflateInit2(&zs, kGzipLevel, Z_DEFLATED, kGzipWindow, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
    throw std::runtime_error("deflateInit2 failed");
  }
  Bytes out(deflateBound(&zs, static_cast<uLong>(raw.size())));
  zs.next_in = raw.data();
  zs.avail_in = static_cast<uInt>(raw.size());
  zs.next_out = out.data();
  zs.avail_out = static_cast<uInt>(out.size());
  const int rc = deflate(&zs, Z_FINISH);
  out.resize(zs.total_out);
  deflateEnd(&zs);
  if (rc != Z_STREAM_END) {
    throw std::runtime_error(std::format("deflate: {}", rc));
  }
  return out;
}

Bytes gunzip(const Bytes& packed) {
  z_stream zs{};
  if (inflateInit2(&zs, kGzipWindow) != Z_OK) {
    throw std::runtime_error("inflateInit2 failed");
  }
  zs.next_in = packed.data();
  zs.avail_in = static_cast<uInt>(packed.size());
  Bytes out;
  int rc = Z_OK;
  while (rc != Z_STREAM_END) {
    out.resize(out.size() + kInflateChunk);
    zs.next_out = out.data() + zs.total_out;
    zs.avail_out = static_cast<uInt>(out.size() - zs.total_out);
    rc = inflate(&zs, Z_NO_FLUSH);
    if (rc != Z_OK && rc != Z_STREAM_END) {
      inflateEnd(&zs);
      throw std::runtime_error(std::format("inflate: {}", rc));
    }
  }
  out.resize(zs.total_out);
  inflateEnd(&zs);
  return out;
}

std::string plain_checksum(const Bytes& raw) {
  return std::format("{}:{}", raw.size(), sha256_hex(raw));
}

Outcome gunzip_kernel(const Args& args) {
  const auto packed = gzip(read_bytes(args));
  auto [raw, times] = measure(args.repeats, [&] { return gunzip(packed); });
  return {plain_checksum(raw), std::move(times), {}};
}

}  // namespace kernels
