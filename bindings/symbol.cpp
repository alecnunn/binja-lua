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

    if (logger) logger->LogDebug("Symbol bindings registered");
}

}  // namespace BinjaLua
