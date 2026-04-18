// Version source of truth for binja-lua.
// The canonical number lives in the project(VERSION ...) call at the
// top of CMakeLists.txt; this header consumes the compile-time macros
// that CMake pipes through target_compile_definitions. See
// docs/versioning.md section 3 for the policy.

#pragma once

#include <string_view>

namespace BinjaLua {

constexpr int kVersionMajor = BINJALUA_VERSION_MAJOR;
constexpr int kVersionMinor = BINJALUA_VERSION_MINOR;
constexpr int kVersionPatch = BINJALUA_VERSION_PATCH;
constexpr std::string_view kVersionString{BINJALUA_VERSION_STRING};

}  // namespace BinjaLua
