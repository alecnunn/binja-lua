// Sol2 configuration header for binja-lua
// This header must be included before sol/sol.hpp
// IMPORTANT: Order matters! Ref<T> traits must be declared before sol2 includes

#pragma once

// Enable all safety features in debug builds
#ifndef NDEBUG
#define SOL_ALL_SAFETIES_ON 1
#endif

// Enable safe exception propagation
#define SOL_EXCEPTIONS_SAFE_PROPAGATION 1

// Enable C++ Lua interop for better integration
#define SOL_USING_CXX_LUA_INTEROP 1

// Use Lua's default allocator
#define SOL_DEFAULT_PASS_ON_ERROR 1

// Include Binary Ninja API FIRST - we need Ref<T> defined before sol2
#include "binaryninjaapi.h"

// Forward declare lua_State (C type from Lua)
extern "C" {
struct lua_State;
}

// Forward declare sol namespace for trait specialization
namespace sol {
template <typename T>
struct unique_usertype_traits;
}

// Specialize unique_usertype_traits for Binary Ninja's Ref<T> smart pointer
// This MUST come before including sol/sol.hpp
namespace sol {

template <typename T>
struct unique_usertype_traits<BinaryNinja::Ref<T>> {
    using type = T;
    using actual_type = BinaryNinja::Ref<T>;

    static bool is_null(lua_State*, const actual_type& ptr) noexcept {
        // Ref<T> has operator! for null checking
        return !ptr;
    }

    static T* get(lua_State*, const actual_type& ptr) noexcept {
        // Ref<T> has operator T*() for implicit conversion to raw pointer
        return static_cast<T*>(ptr);
    }
};

}  // namespace sol

// Now include sol2 - it will use our trait specialization
#include <sol/sol.hpp>

// Standard library includes commonly needed
#include <string>
#include <vector>
#include <cstdint>
#include <fmt/format.h>

using namespace BinaryNinja;
