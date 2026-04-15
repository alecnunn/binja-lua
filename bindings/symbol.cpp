// Sol2 Symbol bindings for binja-lua

#include "common.h"

namespace BinjaLua {

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
            return EnumToString(s.GetType());
        }),

        "type_value", sol::property([](Symbol& s) -> int {
            return static_cast<int>(s.GetType());
        }),

        "raw_name", sol::property([](Symbol& s) -> std::string {
            return s.GetRawName();
        }),

        "namespace_name", sol::property([](Symbol& s) -> std::string {
            return s.GetNameSpace().GetString();
        }),

        "binding", sol::property([](Symbol& s) -> std::string {
            return EnumToString(s.GetBinding());
        }),

        "ordinal", sol::property([](Symbol& s) -> uint64_t {
            return s.GetOrdinal();
        }),

        "auto_defined", sol::property([](Symbol& s) -> bool {
            return s.IsAutoDefined();
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

    // Symbol.new factory - closes the R3b gap where Symbol had
    // sol::no_constructor and BinaryView.define_user_symbol /
    // define_auto_symbol needed a Ref<Symbol> argument that Lua
    // could not build. Shape mirrors python/types.py:421 Symbol
    // constructor: (sym_type, addr, short_name[, full_name[,
    // raw_name[, binding[, ordinal]]]]). sym_type accepts both
    // the short canonical BNSymbolType string (e.g. "Function")
    // and the Python/C verbatim name (e.g. "FunctionSymbol") via
    // the R2 dual-accept EnumFromString helper. The namespace
    // argument is intentionally not exposed because the NameSpace
    // usertype is not bound; callers that need a custom namespace
    // should go through Python.
    lua["Symbol"] = lua.create_table_with(
        "new", [](const std::string& sym_type, sol::object addr_obj,
                   const std::string& short_name,
                   sol::optional<std::string> full_name,
                   sol::optional<std::string> raw_name,
                   sol::optional<std::string> binding_str,
                   sol::optional<uint64_t> ordinal) -> Ref<Symbol> {
            auto type = EnumFromString<BNSymbolType>(sym_type);
            if (!type) return nullptr;
            auto addr = AsAddress(addr_obj);
            if (!addr) return nullptr;

            // Python defaults: full_name = short_name, raw_name =
            // full_name, binding = NoBinding, ordinal = 0.
            std::string fn = full_name.value_or(short_name);
            std::string rn = raw_name.value_or(fn);

            BNSymbolBinding bind = NoBinding;
            if (binding_str) {
                auto b = EnumFromString<BNSymbolBinding>(*binding_str);
                if (b) bind = *b;
            }

            return new Symbol(*type, short_name, fn, rn, *addr, bind,
                              NameSpace(DEFAULT_INTERNAL_NAMESPACE),
                              ordinal.value_or(0));
        }
    );

    if (logger) logger->LogDebug("Symbol bindings registered");
}

}  // namespace BinjaLua
