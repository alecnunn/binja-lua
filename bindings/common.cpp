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

void RegisterSelectionBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Selection bindings");

    lua.new_usertype<Selection>(SELECTION_METATABLE,
        sol::constructors<Selection(), Selection(uint64_t, uint64_t)>(),

        // Properties
        "start_addr", sol::property(
            [](const Selection& s) { return HexAddress(s.start); },
            [](Selection& s, uint64_t v) { s.start = v; }
        ),
        "end_addr", sol::property(
            [](const Selection& s) { return HexAddress(s.end); },
            [](Selection& s, uint64_t v) { s.end = v; }
        ),

        // Methods
        "length", &Selection::length,

        // String representation
        sol::meta_function::to_string, [](const Selection& s) {
            return fmt::format("Selection(0x{:x}-0x{:x}, {} bytes)",
                             s.start, s.end, s.length());
        }
    );

    if (logger) logger->LogDebug("Selection bindings registered");
}

void RegisterSectionBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Section bindings");

    // Register Section type - sol2 will handle Ref<Section> via unique_usertype_traits
    lua.new_usertype<Section>(SECTION_METATABLE,
        sol::no_constructor,

        // Properties - sol2 gives us Section& after dereferencing Ref<Section>
        "name", sol::property([](Section& s) -> std::string {
            return s.GetName();
        }),

        "start_addr", sol::property([](Section& s) -> HexAddress {
            return HexAddress(s.GetStart());
        }),

        "length", sol::property([](Section& s) -> uint64_t {
            return s.GetLength();
        }),

        // Methods
        "permissions", [](sol::this_state ts, Section& s) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            BNSectionSemantics semantics = s.GetSemantics();
            bool isCode = (semantics == ReadOnlyCodeSectionSemantics);
            bool isWritable = (semantics == ReadWriteDataSectionSemantics);
            result["read"] = true;
            result["write"] = isWritable;
            result["execute"] = isCode;
            return result;
        },

        "type", sol::property([](Section& s) -> std::string {
            BNSectionSemantics semantics = s.GetSemantics();
            switch (semantics) {
                case DefaultSectionSemantics: return "default";
                case ReadOnlyCodeSectionSemantics: return "code";
                case ReadOnlyDataSectionSemantics: return "data";
                case ReadWriteDataSectionSemantics: return "data";
                case ExternalSectionSemantics: return "external";
                default: return "unknown";
            }
        }),

        // Comparison operators - must provide to prevent auto-enrollment
        sol::meta_function::equal_to, [](Section& a, Section& b) -> bool {
            // Compare by start address as a reasonable equality check
            return Section::GetObject(&a) == Section::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](Section& s) -> std::string {
            return fmt::format("Section({}, 0x{:x}, {} bytes)",
                             s.GetName(), s.GetStart(), s.GetLength());
        }
    );

    if (logger) logger->LogDebug("Section bindings registered");
}

void RegisterSymbolBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Symbol bindings");

    lua.new_usertype<Symbol>(SYMBOL_METATABLE,
        sol::no_constructor,

        // Properties
        "name", sol::property([](Symbol& s) -> std::string {
            return s.GetFullName();
        }),

        "short_name", sol::property([](Symbol& s) -> std::string {
            return s.GetShortName();
        }),

        "address", sol::property([](Symbol& s) -> HexAddress {
            return HexAddress(s.GetAddress());
        }),

        "type", sol::property([](Symbol& s) -> std::string {
            switch (s.GetType()) {
                case FunctionSymbol: return "Function";
                case ImportAddressSymbol: return "ImportAddress";
                case ImportedFunctionSymbol: return "ImportedFunction";
                case DataSymbol: return "Data";
                case ImportedDataSymbol: return "ImportedData";
                case ExternalSymbol: return "External";
                case LibraryFunctionSymbol: return "LibraryFunction";
                case SymbolicFunctionSymbol: return "SymbolicFunction";
                case LocalLabelSymbol: return "LocalLabel";
                default: return "Unknown";
            }
        }),

        "type_value", sol::property([](Symbol& s) -> int {
            return static_cast<int>(s.GetType());
        }),

        // Comparison operator to prevent auto-enrollment issues
        sol::meta_function::equal_to, [](Symbol& a, Symbol& b) -> bool {
            return Symbol::GetObject(&a) == Symbol::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](Symbol& s) -> std::string {
            return fmt::format("<Symbol: {} @ 0x{:x}>",
                             s.GetShortName(), s.GetAddress());
        }
    );

    if (logger) logger->LogDebug("Symbol bindings registered");
}

// Note: RegisterBasicBlockBindings implemented in basicblock.cpp

// Placeholder implementations for remaining bindings
// These will be filled in during subsequent migration phases
// Note: InstructionBindings implemented in instruction.cpp

// Note: RegisterVariableBindings implemented in variable.cpp

// Note: RegisterFunctionBindings implemented in function.cpp

// Note: RegisterBinaryViewBindings implemented in binaryview.cpp

// Note: RegisterILBindings implemented in il.cpp

} // namespace BinjaLua
