// The builtins group (../builtins-NOTES.md): one kernel per (job, way), named JOB.cpp.WAY; every
// way of a job gets the same input and prints the same checksum. The way lists mirror
// ../builtins/ways.json ("cpp"); the first way of each job is the default.
#pragma once

#include <string_view>

#include "kernels.hpp"

namespace kernels::ways {

// An unknown way throws std::invalid_argument.
Outcome sort_ints(const Args& args, std::string_view job, std::string_view way);     // ways.cpp
Outcome sort_strings(const Args& args, std::string_view job, std::string_view way);  // ways.cpp
Outcome map_ints(const Args& args, std::string_view way);     // ways_maps.cpp
Outcome map_strings(const Args& args, std::string_view way);  // ways_maps.cpp
Outcome join(const Args& args, std::string_view way);         // ways_maps.cpp

}  // namespace kernels::ways
