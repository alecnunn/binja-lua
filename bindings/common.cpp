// Sol2 bindings common implementation for binja-lua

#include "common.h"
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

namespace BinjaLua {

// Static logger storage for access from Lua callbacks
static Ref<Logger> s_storedLogger;

Ref<Logger> GetLogger(sol::state_view lua) {
    (void)lua; // Unused for now - using static storage
    return s_storedLogger;
}

void SetLogger(sol::state_view lua, Ref<Logger> logger) {
    (void)lua; // Unused for now - using static storage
    s_storedLogger = logger;
}

void RegisterAllBindings(lua_State* L, Ref<Logger> logger) {
    if (logger) {
        logger->LogDebug("Registering sol2 bindings...");
    }

    // Create sol2 state view from raw lua_State
    sol::state_view lua(L);

    // Store logger for later access
    SetLogger(lua, logger);

    // Register types in dependency order
    // 1. Simple value types first
    RegisterHexAddressBindings(lua, logger);
    RegisterSelectionBindings(lua, logger);

    // 2. Core BinaryNinja types
    RegisterSectionBindings(lua, logger);
    RegisterSymbolBindings(lua, logger);
    RegisterBasicBlockBindings(lua, logger);

    // 2a. Architecture / CallingConvention / Platform stack.
    // Must come before Function so Function.arch can return a
    // Ref<Architecture> usertype rather than a raw refcount handle.
    RegisterArchitectureBindings(lua, logger);
    RegisterCallingConventionBindings(lua, logger);
    RegisterPlatformBindings(lua, logger);

    // 3. Wrapper types that may reference core types
    RegisterInstructionBindings(lua, logger);
    RegisterVariableBindings(lua, logger);
    RegisterDataVariableBindings(lua, logger);

    // 4. Complex types that use all others
    RegisterFunctionBindings(lua, logger);
    RegisterBinaryViewBindings(lua, logger);

    // 5. IL types
    RegisterILBindings(lua, logger);

    // 6. Type system
    RegisterTypeBindings(lua, logger);

    // 7. Tag system
    RegisterTagBindings(lua, logger);

    // 8. FlowGraph (for reports)
    RegisterFlowGraphBindings(lua, logger);

    // 9. Global utility functions
    RegisterGlobalFunctions(lua, logger);

    // 10. Load optional Lua-side extensions
    LoadLuaAPIExtensions(lua, logger);

    if (logger) {
        logger->LogDebug("Sol2 bindings registration complete");
    }
}

sol::object MetadataToLua(sol::state_view lua, BNMetadata* md) {
    if (!md) {
        return sol::make_object(lua, sol::nil);
    }

    BNMetadataType type = BNMetadataGetType(md);
    switch (type) {
        case BooleanDataType:
            return sol::make_object(lua, BNMetadataGetBoolean(md));

        case StringDataType: {
            char* str = BNMetadataGetString(md);
            if (!str) {
                return sol::make_object(lua, sol::nil);
            }
            sol::object result = sol::make_object(lua, std::string(str));
            BNFreeString(str);
            return result;
        }

        case UnsignedIntegerDataType:
            return sol::make_object(lua, BNMetadataGetUnsignedInteger(md));

        case SignedIntegerDataType:
            return sol::make_object(lua, BNMetadataGetSignedInteger(md));

        case DoubleDataType:
            return sol::make_object(lua, BNMetadataGetDouble(md));

        case RawDataType: {
            size_t size = 0;
            uint8_t* buffer = BNMetadataGetRaw(md, &size);
            if (!buffer) {
                return sol::make_object(lua, std::string());
            }
            // Construct a Lua string from the (pointer, length) pair so
            // embedded null bytes round-trip correctly. BNFreeMetadataRaw
            // must be called regardless of construction success.
            std::string raw(reinterpret_cast<const char*>(buffer), size);
            BNFreeMetadataRaw(buffer);
            return sol::make_object(lua, std::move(raw));
        }

        case KeyValueDataType: {
            BNMetadataValueStore* store = BNMetadataGetValueStore(md);
            if (!store) {
                return sol::make_object(lua, sol::nil);
            }
            sol::table tbl = lua.create_table(0, static_cast<int>(store->size));
            for (size_t i = 0; i < store->size; ++i) {
                // Recursive decode - every nested value comes back as
                // whatever native Lua type MetadataToLua produces.
                tbl[store->keys[i]] = MetadataToLua(lua, store->values[i]);
            }
            sol::object result = sol::make_object(lua, tbl);
            BNFreeMetadataValueStore(store);
            return result;
        }

        case ArrayDataType: {
            size_t size = BNMetadataSize(md);
            sol::table tbl = lua.create_table(static_cast<int>(size), 0);
            for (size_t i = 0; i < size; ++i) {
                BNMetadata* element = BNMetadataGetForIndex(md, i);
                // BNMetadataGetForIndex returns a NEW reference that the
                // caller must free, mirroring Python metadata.py:186.
                tbl[i + 1] = MetadataToLua(lua, element);
                if (element) {
                    BNFreeMetadata(element);
                }
            }
            return sol::make_object(lua, tbl);
        }

        case InvalidDataType:
        default:
            // InvalidDataType is a sentinel in Python (metadata.py:220
            // raises TypeError); from Lua a throw here would collapse
            // the whole call into an error, so return nil instead and
            // let callers decide.
            return sol::make_object(lua, sol::nil);
    }
}

BNMetadata* MetadataFromLua(sol::object value, bool prefer_unsigned) {
    // Booleans must be tested before numbers; sol::is<bool> and
    // sol::is<int64_t> both return true for a Lua bool under some sol2
    // builds, and an accidental bool-to-integer coercion would lose
    // type information on the way through BN.
    if (value.is<bool>()) {
        return BNCreateMetadataBooleanData(value.as<bool>());
    }

    if (value.is<std::string>()) {
        // Lua strings are 8-bit clean; pass them through as StringData.
        // There is no Lua-level "raw bytes" distinction, so Raw is a
        // decode-only variant - encoding always produces StringData.
        const std::string& s = value.as<std::string>();
        return BNCreateMetadataStringData(s.c_str());
    }

    if (value.get_type() == sol::type::number) {
        double d = value.as<double>();
        if (d == std::floor(d) && d >= static_cast<double>(INT64_MIN) &&
            d <= static_cast<double>(INT64_MAX)) {
            if (prefer_unsigned && d >= 0.0) {
                return BNCreateMetadataUnsignedIntegerData(
                    static_cast<uint64_t>(d));
            }
            return BNCreateMetadataSignedIntegerData(
                static_cast<int64_t>(d));
        }
        return BNCreateMetadataDoubleData(d);
    }

    if (!value.is<sol::table>()) {
        return nullptr;
    }

    sol::table tbl = value.as<sol::table>();

    // Detect 1-indexed sequential array vs string-keyed map.
    bool is_array = true;
    size_t expected_idx = 1;
    for (auto& kv : tbl) {
        if (!kv.first.is<size_t>() ||
            kv.first.as<size_t>() != expected_idx) {
            is_array = false;
            break;
        }
        ++expected_idx;
    }

    if (is_array && expected_idx > 1) {
        // ArrayDataType - recurse per element via BNMetadataArrayAppend.
        // Each appended child metadata handle is owned by the array
        // after append, so free children only on failure paths.
        BNMetadata* array = BNCreateMetadataOfType(ArrayDataType);
        if (!array) return nullptr;
        for (size_t i = 1; i < expected_idx; ++i) {
            sol::object element = tbl[i];
            BNMetadata* child = MetadataFromLua(element, prefer_unsigned);
            if (!child) {
                BNFreeMetadata(array);
                return nullptr;
            }
            bool ok = BNMetadataArrayAppend(array, child);
            BNFreeMetadata(child);
            if (!ok) {
                BNFreeMetadata(array);
                return nullptr;
            }
        }
        return array;
    }

    // Empty tables and string-keyed tables -> KeyValueDataType.
    // Build via BNCreateMetadataOfType + BNMetadataSetValueForKey so
    // each value can be recursively encoded.
    BNMetadata* store = BNCreateMetadataOfType(KeyValueDataType);
    if (!store) return nullptr;
    for (auto& kv : tbl) {
        if (!kv.first.is<std::string>()) {
            continue;
        }
        BNMetadata* child = MetadataFromLua(kv.second, prefer_unsigned);
        if (!child) {
            // Skip values we cannot encode rather than aborting the
            // whole store - matches how the previous codec treated
            // unrecognised scalars.
            continue;
        }
        std::string key = kv.first.as<std::string>();
        bool ok = BNMetadataSetValueForKey(store, key.c_str(), child);
        BNFreeMetadata(child);
        if (!ok) {
            BNFreeMetadata(store);
            return nullptr;
        }
    }
    return store;
}

void RegisterGlobalFunctions(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering global functions");

    // Markdown to HTML conversion utility
    lua["markdown_to_html"] = [](const std::string& markdown) -> std::string {
        char* html = BNMarkdownToHTML(markdown.c_str());
        if (!html) return "";
        std::string result(html);
        BNFreeString(html);
        return result;
    };

    if (logger) logger->LogDebug("Global functions registered");
}

void LoadLuaAPIExtensions(sol::state_view lua, Ref<Logger> logger) {
    // Find the lua-api directory relative to the plugin location
    std::vector<std::string> searchPaths;

    // Try user plugin directory first
    char* userPluginDir = BNGetUserPluginDirectory();
    if (userPluginDir) {
        searchPaths.push_back(std::string(userPluginDir) + "/binja-lua/lua-api");
        BNFreeString(userPluginDir);
    }

    // Try bundled plugin directory
    char* bundledPluginDir = BNGetBundledPluginDirectory();
    if (bundledPluginDir) {
        searchPaths.push_back(std::string(bundledPluginDir) + "/binja-lua/lua-api");
        BNFreeString(bundledPluginDir);
    }

    std::string luaApiPath;
    for (const auto& path : searchPaths) {
        std::string initPath = path + "/init.lua";
        if (std::filesystem::exists(initPath)) {
            luaApiPath = path;
            break;
        }
    }

    if (luaApiPath.empty()) {
        if (logger) {
            logger->LogDebug("Lua API extensions not found, using core bindings only");
        }
        return;
    }

    // Add lua-api directory to package.path
    std::string currentPath = lua["package"]["path"];
    std::string newPath = luaApiPath + "/?.lua;" + currentPath;
    lua["package"]["path"] = newPath;

    // Load init.lua which loads all extensions
    std::string initPath = luaApiPath + "/init.lua";
    auto result = lua.safe_script_file(initPath, sol::script_pass_on_error);

    if (!result.valid()) {
        sol::error err = result;
        if (logger) {
            logger->LogWarn("Failed to load Lua API extensions: %s", err.what());
        }
    } else {
        if (logger) {
            logger->LogInfo("Lua API extensions loaded successfully");
        }
    }
}

// Placeholder implementations - will be filled in during migration phases

void RegisterHexAddressBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering HexAddress bindings");

    lua.new_usertype<HexAddress>(HEXADDRESS_METATABLE,
        sol::constructors<HexAddress(), HexAddress(uint64_t)>(),

        // Value access
        "value", &HexAddress::value,

        // Arithmetic metamethods
        sol::meta_function::addition,
        sol::overload(
            [](const HexAddress& a, int64_t b) { return HexAddress(a.value + b); },
            [](int64_t a, const HexAddress& b) { return HexAddress(a + b.value); }
        ),
        sol::meta_function::subtraction,
        sol::overload(
            [](const HexAddress& a, int64_t b) { return HexAddress(a.value - b); },
            [](const HexAddress& a, const HexAddress& b) {
                return static_cast<int64_t>(a.value) - static_cast<int64_t>(b.value);
            }
        ),

        // Comparison metamethods
        sol::meta_function::equal_to, &HexAddress::operator==,
        sol::meta_function::less_than, &HexAddress::operator<,
        sol::meta_function::less_than_or_equal_to, &HexAddress::operator<=,

        // String representation (hex format for smartlinks)
        sol::meta_function::to_string, [](const HexAddress& addr) {
            return fmt::format("0x{:x}", addr.value);
        }
    );

    if (logger) logger->LogDebug("HexAddress bindings registered");
}

// Note: RegisterSelectionBindings implemented in selection.cpp
// Note: RegisterSectionBindings implemented in section.cpp
// Note: RegisterSymbolBindings implemented in symbol.cpp
// Note: RegisterBasicBlockBindings implemented in basicblock.cpp
// Note: RegisterInstructionBindings implemented in instruction.cpp

// Note: RegisterVariableBindings implemented in variable.cpp

// Note: RegisterFunctionBindings implemented in function.cpp

// Note: RegisterBinaryViewBindings implemented in binaryview.cpp

// Note: RegisterILBindings implemented in il.cpp

} // namespace BinjaLua
