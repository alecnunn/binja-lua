// Sol2 DataVariable bindings for binja-lua

#include "common.h"

namespace BinjaLua {

// DataVariableWrapper implementation

DataVariableWrapper::DataVariableWrapper(uint64_t addr, Ref<BinaryView> bv,
                                         const Confidence<Ref<Type>>& t,
                                         bool autoDisco)
    : address(addr), view(bv), type(t), autoDiscovered(autoDisco) {}

DataVariableWrapper::DataVariableWrapper(const DataVariableWrapper& other)
    : address(other.address), view(other.view), type(other.type),
      autoDiscovered(other.autoDiscovered) {}

DataVariableWrapper::~DataVariableWrapper() {}

std::string DataVariableWrapper::GetTypeName() const {
    Ref<Type> typeRef = type.GetValue();
    if (typeRef) {
        return typeRef->GetString();
    }
    return "<unknown>";
}

std::string DataVariableWrapper::GetName() const {
    if (!view) return "";

    // Try to get symbol at this address
    Ref<Symbol> sym = view->GetSymbolByAddress(address);
    if (sym) {
        return sym->GetShortName();
    }
    return "";
}

void RegisterDataVariableBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering DataVariable bindings");

    lua.new_usertype<DataVariableWrapper>(DATAVARIABLE_METATABLE,
        sol::no_constructor,

        // Properties
        "address", sol::property([](const DataVariableWrapper& dv) -> HexAddress {
            return HexAddress(dv.address);
        }),

        "type", sol::property([](const DataVariableWrapper& dv) -> std::string {
            return dv.GetTypeName();
        }),

        "auto_discovered", sol::property([](const DataVariableWrapper& dv) -> bool {
            return dv.autoDiscovered;
        }),

        "name", sol::property([](const DataVariableWrapper& dv) -> std::string {
            return dv.GetName();
        }),

        "type_confidence", sol::property([](const DataVariableWrapper& dv) -> uint8_t {
            return dv.type.GetConfidence();
        }),

        // Comparison
        sol::meta_function::equal_to,
        [](const DataVariableWrapper& a, const DataVariableWrapper& b) -> bool {
            return a.address == b.address;
        },

        // String representation
        sol::meta_function::to_string,
        [](const DataVariableWrapper& dv) -> std::string {
            std::string name = dv.GetName();
            if (name.empty()) {
                return fmt::format("<DataVariable @ 0x{:x}: {}>",
                                   dv.address, dv.GetTypeName());
            }
            return fmt::format("<DataVariable '{}' @ 0x{:x}: {}>",
                               name, dv.address, dv.GetTypeName());
        }
    );

    if (logger) logger->LogDebug("DataVariable bindings registered");
}

} // namespace BinjaLua
