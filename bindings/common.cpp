// Sol2 bindings common implementation for binja-lua

#include "common.h"
#include <filesystem>

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
