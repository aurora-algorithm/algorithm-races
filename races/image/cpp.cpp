// Race: image (cpp)
// Extracted from suite/cpp/src/image.cpp — build and run via suite/run.py

// image: decode a 24 MP JPEG, resize to 1080 px wide, encode JPEG quality 80, with the stb
// single-file libraries (stb_image, stb_image_resize2 with its default filter, stb_image_write)
// (../SPEC.md section 11).
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stb/stb_image_resize2.h>
#include <stb/stb_image_write.h>

#include <cstdint>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "inputs.hpp"
#include "work.hpp"

namespace kernels {
namespace {

constexpr std::size_t kPhotos = 8;
constexpr int kTargetWidth = 1080;
constexpr int kJpegQuality = 80;
constexpr int kChannels = 3;

struct Converted {
  int width = 0;
  int height = 0;
  std::size_t out_bytes = 0;
};

void append(void* context, void* data, int size) {
  auto* out = static_cast<std::vector<std::uint8_t>*>(context);
  const auto* bytes = static_cast<const std::uint8_t*>(data);
  out->insert(out->end(), bytes, bytes + size);
}

Converted convert_all(const std::vector<std::string>& photos) {
  Converted c;
  for (const auto& data : photos) {
    int w = 0;
    int h = 0;
    int channels = 0;
    const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
        stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(data.data()),
                              static_cast<int>(data.size()), &w, &h, &channels, kChannels),
        &stbi_image_free);
    if (!pixels) {
      throw std::runtime_error(std::format("stb_image: {}", stbi_failure_reason()));
    }
    const int height = (h * kTargetWidth + w / 2) / w;
    std::vector<std::uint8_t> small(static_cast<std::size_t>(kTargetWidth) * height * kChannels);
    if (stbir_resize_uint8_linear(pixels.get(), w, h, 0, small.data(), kTargetWidth, height, 0,
                                  STBIR_RGB) == nullptr) {
      throw std::runtime_error("stb_image_resize2 failed");
    }
    std::vector<std::uint8_t> out;
    if (stbi_write_jpg_to_func(append, &out, kTargetWidth, height, kChannels, small.data(),
                               kJpegQuality) == 0) {
      throw std::runtime_error("stb_image_write failed");
    }
    c = {kTargetWidth, height, c.out_bytes + out.size()};
  }
  return c;
}

}  // namespace

Outcome image_kernel(const Args& args) {
  std::vector<std::string> photos(args.size);
  for (std::size_t i = 0; i < args.size; ++i) {
    photos[i] = read_file(args.data / "photos" / std::format("photo-{}.jpg", i % kPhotos));
  }
  auto [c, times] = measure(args.repeats, [&] { return convert_all(photos); });
  return {std::format("{}:{}x{}", photos.size(), c.width, c.height), std::move(times),
          std::format(R"({{"out_bytes":{}}})", c.out_bytes)};
}

}  // namespace kernels
