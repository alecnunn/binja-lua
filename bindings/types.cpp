// Sol2 Type System bindings for binja-lua

#include "common.h"

namespace BinjaLua {

// Helper to convert BNTypeClass to string
static std::string TypeClassToString(BNTypeClass tc) {
    switch (tc) {
        case VoidTypeClass: return "Void";
        case BoolTypeClass: return "Bool";
        case IntegerTypeClass: return "Integer";
        case FloatTypeClass: return "Float";
        case StructureTypeClass: return "Structure";
        case EnumerationTypeClass: return "Enumeration";
        case PointerTypeClass: return "Pointer";
        case ArrayTypeClass: return "Array";
        case FunctionTypeClass: return "Function";
        case VarArgsTypeClass: return "VarArgs";
        case ValueTypeClass: return "Value";
        case NamedTypeReferenceClass: return "NamedTypeReference";
        case WideCharTypeClass: return "WideChar";
        default: return "Unknown";
    }
}

void RegisterTypeBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Type bindings (experimental)");

    // Base Type usertype
    lua.new_usertype<Type>(TYPE_METATABLE,
        sol::no_constructor,

        // Core properties
        "type_class", sol::property([](Type& t) -> std::string {
            return TypeClassToString(t.GetClass());
        }),

        "type_class_value", sol::property([](Type& t) -> int {
            return static_cast<int>(t.GetClass());
        }),

        "width", sol::property([](Type& t) -> uint64_t {
            return t.GetWidth();
        }),

        "alignment", sol::property([](Type& t) -> size_t {
            return t.GetAlignment();
        }),

        "name", sol::property([](Type& t) -> std::string {
            auto name = t.GetTypeName();
            return name.GetString();
        }),

        // Type queries
        "is_void", sol::property([](Type& t) -> bool {
            return t.GetClass() == VoidTypeClass;
        }),

        "is_bool", sol::property([](Type& t) -> bool {
            return t.GetClass() == BoolTypeClass;
        }),

        "is_integer", sol::property([](Type& t) -> bool {
            return t.GetClass() == IntegerTypeClass;
        }),

        "is_float", sol::property([](Type& t) -> bool {
            return t.GetClass() == FloatTypeClass;
        }),

        "is_structure", sol::property([](Type& t) -> bool {
            return t.GetClass() == StructureTypeClass;
        }),

        "is_enumeration", sol::property([](Type& t) -> bool {
            return t.GetClass() == EnumerationTypeClass;
        }),

        "is_pointer", sol::property([](Type& t) -> bool {
            return t.GetClass() == PointerTypeClass;
        }),

        "is_array", sol::property([](Type& t) -> bool {
            return t.GetClass() == ArrayTypeClass;
        }),

        "is_function", sol::property([](Type& t) -> bool {
            return t.GetClass() == FunctionTypeClass;
        }),

        "is_signed", sol::property([](Type& t) -> bool {
            return t.IsSigned().GetValue();
        }),

        "is_const", sol::property([](Type& t) -> bool {
            return t.IsConst().GetValue();
        }),

        "is_volatile", sol::property([](Type& t) -> bool {
            return t.IsVolatile().GetValue();
        }),

        // Pointer-specific - returns target type as Ref<Type>
        "target", [](Type& t) -> Ref<Type> {
            if (t.GetClass() == PointerTypeClass) {
                auto child = t.GetChildType();
                return child.GetValue();
            }
            return nullptr;
        },

        // Array-specific
        "element_count", sol::property([](Type& t) -> uint64_t {
            if (t.GetClass() == ArrayTypeClass) {
                return t.GetElementCount();
            }
            return 0;
        }),

        // Structure members (returns table for structures)
        "members", [](sol::this_state ts, Type& t) -> sol::object {
            sol::state_view lua(ts);
            if (t.GetClass() != StructureTypeClass) {
                return sol::make_object(lua, sol::nil);
            }

            Ref<Structure> structure = t.GetStructure();
            if (!structure) {
                return sol::make_object(lua, sol::nil);
            }

            sol::table result = lua.create_table();
            auto members = structure->GetMembers();
            for (size_t i = 0; i < members.size(); i++) {
                sol::table member = lua.create_table();
                member["name"] = members[i].name;
                member["offset"] = members[i].offset;
                if (members[i].type.GetValue()) {
                    member["type"] = members[i].type.GetValue()->GetString();
                    member["width"] = members[i].type.GetValue()->GetWidth();
                } else {
                    member["type"] = "unknown";
                    member["width"] = 0;
                }
                switch (members[i].access) {
                    case NoAccess: member["access"] = "none"; break;
                    case PrivateAccess: member["access"] = "private"; break;
                    case ProtectedAccess: member["access"] = "protected"; break;
                    case PublicAccess: member["access"] = "public"; break;
                    default: member["access"] = "unknown"; break;
                }
                result[i + 1] = member;
            }
            return sol::make_object(lua, result);
        },

        // Get structure member by name
        "get_member_by_name", [](sol::this_state ts, Type& t,
                                 const std::string& name) -> sol::object {
            sol::state_view lua(ts);
            if (t.GetClass() != StructureTypeClass) {
                return sol::make_object(lua, sol::nil);
            }

            Ref<Structure> structure = t.GetStructure();
            if (!structure) {
                return sol::make_object(lua, sol::nil);
            }

            StructureMember result;
            if (!structure->GetMemberByName(name, result)) {
                return sol::make_object(lua, sol::nil);
            }

            sol::table member = lua.create_table();
            member["name"] = result.name;
            member["offset"] = result.offset;
            if (result.type.GetValue()) {
                member["type"] = result.type.GetValue()->GetString();
                member["width"] = result.type.GetValue()->GetWidth();
            } else {
                member["type"] = "unknown";
                member["width"] = 0;
            }
            switch (result.access) {
                case NoAccess: member["access"] = "none"; break;
                case PrivateAccess: member["access"] = "private"; break;
                case ProtectedAccess: member["access"] = "protected"; break;
                case PublicAccess: member["access"] = "public"; break;
                default: member["access"] = "unknown"; break;
            }
            return sol::make_object(lua, member);
        },

        // Get structure member at offset
        "get_member_at_offset", [](sol::this_state ts, Type& t,
                                   int64_t offset) -> sol::object {
            sol::state_view lua(ts);
            if (t.GetClass() != StructureTypeClass) {
                return sol::make_object(lua, sol::nil);
            }

            Ref<Structure> structure = t.GetStructure();
            if (!structure) {
                return sol::make_object(lua, sol::nil);
            }

            StructureMember result;
            if (!structure->GetMemberAtOffset(offset, result)) {
                return sol::make_object(lua, sol::nil);
            }

            sol::table member = lua.create_table();
            member["name"] = result.name;
            member["offset"] = result.offset;
            if (result.type.GetValue()) {
                member["type"] = result.type.GetValue()->GetString();
                member["width"] = result.type.GetValue()->GetWidth();
            } else {
                member["type"] = "unknown";
                member["width"] = 0;
            }
            switch (result.access) {
                case NoAccess: member["access"] = "none"; break;
                case PrivateAccess: member["access"] = "private"; break;
                case ProtectedAccess: member["access"] = "protected"; break;
                case PublicAccess: member["access"] = "public"; break;
                default: member["access"] = "unknown"; break;
            }
            return sol::make_object(lua, member);
        },

        // Enumeration members (returns table for enumerations)
        "enum_members", [](sol::this_state ts, Type& t) -> sol::object {
            sol::state_view lua(ts);
            if (t.GetClass() != EnumerationTypeClass) {
                return sol::make_object(lua, sol::nil);
            }

            Ref<Enumeration> enumeration = t.GetEnumeration();
            if (!enumeration) {
                return sol::make_object(lua, sol::nil);
            }

            sol::table result = lua.create_table();
            auto members = enumeration->GetMembers();
            for (size_t i = 0; i < members.size(); i++) {
                sol::table member = lua.create_table();
                member["name"] = members[i].name;
                member["value"] = members[i].value;
                member["is_default"] = members[i].isDefault;
                result[i + 1] = member;
            }
            return sol::make_object(lua, result);
        },

        // Function type properties
        "return_type", sol::property([](Type& t) -> std::string {
            if (t.GetClass() != FunctionTypeClass) {
                return "";
            }
            auto retType = t.GetChildType();
            if (retType.GetValue()) {
                return retType.GetValue()->GetString();
            }
            return "void";
        }),

        "parameters", [](sol::this_state ts, Type& t) -> sol::object {
            sol::state_view lua(ts);
            if (t.GetClass() != FunctionTypeClass) {
                return sol::make_object(lua, sol::nil);
            }

            sol::table result = lua.create_table();
            auto params = t.GetParameters();
            for (size_t i = 0; i < params.size(); i++) {
                sol::table param = lua.create_table();
                param["name"] = params[i].name;
                if (params[i].type.GetValue()) {
                    param["type"] = params[i].type.GetValue()->GetString();
                } else {
                    param["type"] = "unknown";
                }
                param["has_default"] = params[i].defaultLocation;
                result[i + 1] = param;
            }
            return sol::make_object(lua, result);
        },

        "has_variable_arguments", sol::property([](Type& t) -> bool {
            if (t.GetClass() != FunctionTypeClass) {
                return false;
            }
            return t.HasVariableArguments().GetValue();
        }),

        "can_return", sol::property([](Type& t) -> bool {
            if (t.GetClass() != FunctionTypeClass) {
                return true;
            }
            return t.CanReturn().GetValue();
        }),

        // Get string representation of the type
        "get_string", [](Type& t) -> std::string {
            return t.GetString();
        },

        // Comparison
        sol::meta_function::equal_to, [](Type& a, Type& b) -> bool {
            return Type::GetObject(&a) == Type::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](Type& t) -> std::string {
            return fmt::format("<Type: {}>", t.GetString());
        }
    );

    if (logger) logger->LogDebug("Type bindings registered");
}

} // namespace BinjaLua
