// Sol2 Variable bindings for binja-lua

#include "common.h"

namespace BinjaLua {

// VariableWrapper Implementation

VariableWrapper::VariableWrapper(const BNVariable& var, Ref<Function> func)
    : bnVar(var), function(func), nameResolved(false), typeResolved(false) {
}

VariableWrapper::VariableWrapper(const VariableWrapper& other)
    : bnVar(other.bnVar), function(other.function),
      cachedName(other.cachedName), cachedTypeName(other.cachedTypeName),
      nameResolved(other.nameResolved), typeResolved(other.typeResolved) {
}

VariableWrapper::~VariableWrapper() {
}

void VariableWrapper::ResolveName() const {
    if (nameResolved) return;

    if (function) {
        cachedName = function->GetVariableNameOrDefault(bnVar);
        if (cachedName.empty()) {
            cachedName = GetSourceTypeString() + "_" + std::to_string(bnVar.index);
        }
    } else {
        cachedName = "var_" + std::to_string(bnVar.index);
    }

    nameResolved = true;
}

void VariableWrapper::ResolveType() const {
    if (typeResolved) return;

    if (function) {
        Confidence<Ref<Type>> type = function->GetVariableType(bnVar);
        if (type.GetValue()) {
            cachedTypeName = type.GetValue()->GetStringBeforeName();
            if (cachedTypeName.empty()) {
                cachedTypeName = type.GetValue()->GetString();
            }
        }
    }

    if (cachedTypeName.empty()) {
        cachedTypeName = "<unknown>";
    }

    typeResolved = true;
}

std::string VariableWrapper::GetSourceTypeString() const {
    switch (bnVar.type) {
        case BNVariableSourceType::StackVariableSourceType:
            return "local";
        case BNVariableSourceType::RegisterVariableSourceType:
            return "register";
        case BNVariableSourceType::FlagVariableSourceType:
            return "flag";
        default:
            return "unknown";
    }
}

std::string VariableWrapper::GetName() const {
    ResolveName();
    return cachedName;
}

std::string VariableWrapper::GetTypeName() const {
    ResolveType();
    return cachedTypeName;
}

// Sol2 Binding Registration

void RegisterVariableBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Variable bindings");

    lua.new_usertype<VariableWrapper>(VARIABLE_METATABLE,
        // No public constructor - created from Function:variables() etc.
        sol::no_constructor,

        // Properties - lazy-loaded via ResolveName/ResolveType
        "name", sol::property([](const VariableWrapper& v) -> std::string {
            return v.GetName();
        }),

        "type", sol::property([](const VariableWrapper& v) -> std::string {
            return v.GetTypeName();
        }),

        // Alias for type (commonly used)
        "type_name", sol::property([](const VariableWrapper& v) -> std::string {
            return v.GetTypeName();
        }),

        "index", sol::property([](const VariableWrapper& v) -> uint32_t {
            return v.bnVar.index;
        }),

        // Methods
        "location", [](sol::this_state ts, const VariableWrapper& v) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            std::string locType;
            if (v.bnVar.type == BNVariableSourceType::StackVariableSourceType) {
                locType = "stack";
            } else if (v.bnVar.type == BNVariableSourceType::RegisterVariableSourceType) {
                locType = "register";
            } else if (v.bnVar.type == BNVariableSourceType::FlagVariableSourceType) {
                locType = "flag";
            } else {
                locType = "unknown";
            }

            result["type"] = locType;
            result["offset"] = static_cast<int64_t>(v.bnVar.storage);

            return result;
        },

        "source_type", sol::property([](const VariableWrapper& v) -> std::string {
            if (v.function) {
                auto params = v.function->GetParameterVariables();
                if (params.GetValue().size() > 0) {
                    for (const auto& param : params.GetValue()) {
                        if (param.type == v.bnVar.type &&
                            param.index == v.bnVar.index &&
                            param.storage == v.bnVar.storage) {
                            return "parameter";
                        }
                    }
                }
            }

            if (v.bnVar.type == BNVariableSourceType::StackVariableSourceType ||
                v.bnVar.type == BNVariableSourceType::RegisterVariableSourceType) {
                return "local";
            }

            return "unknown";
        }),

        // Mutating methods
        "set_name", [](VariableWrapper& v, const std::string& name) -> bool {
            if (!v.function) {
                return false;
            }

            Confidence<Ref<Type>> type = v.function->GetVariableType(v.bnVar);

            if (v.function->IsVariableUserDefinded(v.bnVar)) {
                v.function->DeleteUserVariable(v.bnVar);
            }

            v.function->CreateUserVariable(v.bnVar, type, name);

            Ref<BinaryView> bv = v.function->GetView();
            bv->UpdateAnalysisAndWait();

            v.nameResolved = false;
            v.typeResolved = false;

            return true;
        },

        "set_type", [](VariableWrapper& v, const std::string& typeStr) -> bool {
            if (!v.function) {
                return false;
            }

            // Parse type string to create Type object
            Ref<Type> type;
            if (typeStr == "int32_t" || typeStr == "int") {
                type = Type::IntegerType(4, true);
            } else if (typeStr == "uint32_t" || typeStr == "unsigned int") {
                type = Type::IntegerType(4, false);
            } else if (typeStr == "int64_t" || typeStr == "long long") {
                type = Type::IntegerType(8, true);
            } else if (typeStr == "uint64_t" || typeStr == "unsigned long long") {
                type = Type::IntegerType(8, false);
            } else if (typeStr == "int16_t" || typeStr == "short") {
                type = Type::IntegerType(2, true);
            } else if (typeStr == "uint16_t" || typeStr == "unsigned short") {
                type = Type::IntegerType(2, false);
            } else if (typeStr == "int8_t" || typeStr == "char") {
                type = Type::IntegerType(1, true);
            } else if (typeStr == "uint8_t" || typeStr == "unsigned char") {
                type = Type::IntegerType(1, false);
            } else if (typeStr == "char*" || typeStr == "const char*") {
                type = Type::PointerType(v.function->GetArchitecture(),
                                         Type::IntegerType(1, true));
            } else if (typeStr == "void*") {
                type = Type::PointerType(v.function->GetArchitecture(),
                                         Type::VoidType());
            } else if (typeStr == "void") {
                type = Type::VoidType();
            } else {
                // Default to int32_t for unknown types
                type = Type::IntegerType(4, true);
            }

            if (!type) {
                return false;
            }

            v.ResolveName();
            std::string currentName = v.cachedName;

            if (v.function->IsVariableUserDefinded(v.bnVar)) {
                v.function->DeleteUserVariable(v.bnVar);
            }

            v.function->CreateUserVariable(v.bnVar, Confidence<Ref<Type>>(type), currentName);

            Ref<BinaryView> bv = v.function->GetView();
            bv->UpdateAnalysisAndWait();

            v.nameResolved = false;
            v.typeResolved = false;

            return true;
        },

        // Equality based on variable identity
        sol::meta_function::equal_to, [](const VariableWrapper& a,
                                         const VariableWrapper& b) -> bool {
            return a.bnVar.type == b.bnVar.type &&
                   a.bnVar.index == b.bnVar.index &&
                   a.bnVar.storage == b.bnVar.storage;
        },

        // String representation
        sol::meta_function::to_string, [](const VariableWrapper& v) -> std::string {
            v.ResolveName();
            v.ResolveType();

            if (v.bnVar.type == BNVariableSourceType::StackVariableSourceType) {
                return fmt::format("<Variable: {} ({}) @ stack{:+}>",
                                 v.cachedName, v.cachedTypeName,
                                 static_cast<int64_t>(v.bnVar.storage));
            } else if (v.bnVar.type == BNVariableSourceType::RegisterVariableSourceType) {
                return fmt::format("<Variable: {} ({}) @ register>",
                                 v.cachedName, v.cachedTypeName);
            } else {
                return fmt::format("<Variable: {} ({}) @ {}>",
                                 v.cachedName, v.cachedTypeName,
                                 v.GetSourceTypeString());
            }
        }
    );

    if (logger) logger->LogDebug("Variable bindings registered");
}

} // namespace BinjaLua
