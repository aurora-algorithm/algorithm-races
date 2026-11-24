// Library kernels that live in their own files, so each heavy header is compiled once.
#pragma once

#include <cstdint>
#include <span>
#include <string>

#include "kernels.hpp"

namespace kernels {

Outcome sha256_kernel(const Args& args);                      // sha.cpp (Botan)
Outcome json_kernel(const Args& args);                        // json.cpp (nlohmann/json)
Outcome regex_ctre_kernel(const Args& args);                  // regex_ctre.cpp (CTRE)
std::string sha256_hex(std::span<const std::uint8_t> bytes);  // sha.cpp

inline constexpr std::string_view kLogPattern =
    R"((GET|POST) /api/v[0-9]/users/([0-9]+) (5[0-9][0-9]))";

}  // namespace kernels
