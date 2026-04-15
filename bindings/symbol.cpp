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

}  // namespace BinjaLua
