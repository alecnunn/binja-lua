// Sol2 bindings common header for binja-lua
// Contains metatable names, type traits, and registration declarations

#pragma once

#include "sol_config.h"

#include <optional>

namespace BinjaLua {

// Metatable names - must match existing names for Lua API extension compatibility
// These are used by lua-api/*.lua via debug.getregistry()
constexpr const char* BINARYVIEW_METATABLE = "BinaryNinja.BinaryView";
constexpr const char* FUNCTION_METATABLE = "BinaryNinja.Function";
constexpr const char* BASICBLOCK_METATABLE = "BinaryNinja.BasicBlock";
constexpr const char* SYMBOL_METATABLE = "BinaryNinja.Symbol";
constexpr const char* INSTRUCTION_METATABLE = "BinaryNinja.Instruction";
constexpr const char* VARIABLE_METATABLE = "BinaryNinja.Variable";
constexpr const char* SECTION_METATABLE = "BinaryNinja.Section";
constexpr const char* SELECTION_METATABLE = "BinaryNinja.Selection";
constexpr const char* LLIL_METATABLE = "BinaryNinja.LLIL";
constexpr const char* MLIL_METATABLE = "BinaryNinja.MLIL";
constexpr const char* HLIL_METATABLE = "BinaryNinja.HLIL";
constexpr const char* HEXADDRESS_METATABLE = "BinaryNinja.HexAddress";
constexpr const char* DATAVARIABLE_METATABLE = "BinaryNinja.DataVariable";
constexpr const char* TYPE_METATABLE = "BinaryNinja.Type";
constexpr const char* FLOWGRAPH_METATABLE = "BinaryNinja.FlowGraph";
constexpr const char* FLOWGRAPHNODE_METATABLE = "BinaryNinja.FlowGraphNode";
constexpr const char* ARCHITECTURE_METATABLE = "BinaryNinja.Architecture";
constexpr const char* CALLINGCONVENTION_METATABLE =
    "BinaryNinja.CallingConvention";
constexpr const char* PLATFORM_METATABLE = "BinaryNinja.Platform";
constexpr const char* SETTINGS_METATABLE = "BinaryNinja.Settings";

// Logger key for storing in Lua registry
constexpr const char* LOGGER_REGISTRY_KEY = "__binja_logger";

// Forward declarations for wrapper types
struct Selection;
class InstructionWrapper;
class VariableWrapper;
struct HexAddress;
class DataVariableWrapper;

// Get logger from Lua state
Ref<Logger> GetLogger(sol::state_view lua);

// Store logger in Lua state
void SetLogger(sol::state_view lua, Ref<Logger> logger);

// Main registration function - takes raw lua_State* for compatibility with provider
void RegisterAllBindings(lua_State* L, Ref<Logger> logger);

// Individual binding registration functions
void RegisterBinaryViewBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterFunctionBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterBasicBlockBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterSymbolBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterInstructionBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterVariableBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterSectionBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterSelectionBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterILBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterHexAddressBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterDataVariableBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterTypeBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterTagBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterFlowGraphBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterArchitectureBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterCallingConventionBindings(sol::state_view lua,
                                        Ref<Logger> logger);
void RegisterPlatformBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterSettingsBindings(sol::state_view lua, Ref<Logger> logger);
void RegisterGlobalFunctions(sol::state_view lua, Ref<Logger> logger);

// Load optional Lua API extensions (lua-api/*.lua)
void LoadLuaAPIExtensions(sol::state_view lua, Ref<Logger> logger);

// Selection type - represents a range selection in Binary Ninja
struct Selection {
    uint64_t start;
    uint64_t end;

    Selection() : start(0), end(0) {}
    Selection(uint64_t s, uint64_t e) : start(s), end(e) {}

    uint64_t length() const { return end > start ? end - start : 0; }
};

// HexAddress type - wraps addresses for hex-formatted output (smartlinks)
struct HexAddress {
    uint64_t value;

    HexAddress() : value(0) {}
    explicit HexAddress(uint64_t v) : value(v) {}

    // Arithmetic operations
    HexAddress operator+(int64_t offset) const { return HexAddress(value + offset); }
    HexAddress operator-(int64_t offset) const { return HexAddress(value - offset); }
    int64_t operator-(const HexAddress& other) const {
        return static_cast<int64_t>(value) - static_cast<int64_t>(other.value);
    }

    bool operator==(const HexAddress& other) const { return value == other.value; }
    bool operator!=(const HexAddress& other) const { return value != other.value; }
    bool operator<(const HexAddress& other) const { return value < other.value; }
    bool operator<=(const HexAddress& other) const { return value <= other.value; }
    bool operator>(const HexAddress& other) const { return value > other.value; }
    bool operator>=(const HexAddress& other) const { return value >= other.value; }
};

// Convert an std::vector into a 1-indexed Lua sequence table using a
// per-element projection. The projection receives const T& and returns any
// value sol2 can assign into a table slot (primitive, std::string, Ref<U>,
// or another table).
template <typename T, typename F>
sol::table ToLuaTable(sol::this_state ts, const std::vector<T>& items,
                      F&& project) {
    sol::state_view lua(ts);
    sol::table result = lua.create_table(static_cast<int>(items.size()), 0);
    for (size_t i = 0; i < items.size(); ++i) {
        result[i + 1] = project(items[i]);
    }
    return result;
}

// Convenience overload: pass items through unchanged. Matches the most
// common "result[i + 1] = items[i];" loop.
template <typename T>
sol::table ToLuaTable(sol::this_state ts, const std::vector<T>& items) {
    return ToLuaTable(ts, items,
                      [](const T& item) -> const T& { return item; });
}

// Enum-to-string helpers. Vocabularies follow the strings already in use
// across the bindings; consolidating them here lets new call sites drop a
// single helper in place instead of re-writing per-enum switch blocks.

inline const char* EnumToString(BNBranchType type) {
    switch (type) {
        case UnconditionalBranch: return "unconditional";
        case FalseBranch:         return "false";
        case TrueBranch:          return "true";
        case CallDestination:     return "call";
        case FunctionReturn:      return "return";
        case SystemCall:          return "syscall";
        case IndirectBranch:      return "indirect";
        case ExceptionBranch:     return "exception";
        case UnresolvedBranch:    return "unresolved";
        case UserDefinedBranch:   return "user_defined";
    }
    return "unknown";
}

inline const char* EnumToString(BNSymbolType type) {
    switch (type) {
        case FunctionSymbol:         return "Function";
        case ImportAddressSymbol:    return "ImportAddress";
        case ImportedFunctionSymbol: return "ImportedFunction";
        case DataSymbol:             return "Data";
        case ImportedDataSymbol:     return "ImportedData";
        case ExternalSymbol:         return "External";
        case LibraryFunctionSymbol:  return "LibraryFunction";
        case SymbolicFunctionSymbol: return "SymbolicFunction";
        case LocalLabelSymbol:       return "LocalLabel";
    }
    return "Unknown";
}

inline const char* EnumToString(BNSectionSemantics semantics) {
    switch (semantics) {
        case DefaultSectionSemantics:       return "default";
        case ReadOnlyCodeSectionSemantics:  return "code";
        case ReadOnlyDataSectionSemantics:  return "ro_data";
        case ReadWriteDataSectionSemantics: return "rw_data";
        case ExternalSectionSemantics:      return "external";
    }
    return "unknown";
}

inline const char* EnumToString(BNTypeClass tc) {
    switch (tc) {
        case VoidTypeClass:               return "Void";
        case BoolTypeClass:               return "Bool";
        case IntegerTypeClass:            return "Integer";
        case FloatTypeClass:              return "Float";
        case StructureTypeClass:          return "Structure";
        case EnumerationTypeClass:        return "Enumeration";
        case PointerTypeClass:            return "Pointer";
        case ArrayTypeClass:              return "Array";
        case FunctionTypeClass:           return "Function";
        case VarArgsTypeClass:            return "VarArgs";
        case ValueTypeClass:              return "Value";
        case NamedTypeReferenceClass:     return "NamedTypeReference";
        case WideCharTypeClass:           return "WideChar";
    }
    return "Unknown";
}

inline const char* EnumToString(BNAnalysisState state) {
    switch (state) {
        case InitialState:         return "initial";
        case HoldState:            return "hold";
        case IdleState:            return "idle";
        case DiscoveryState:       return "discovery";
        case DisassembleState:     return "disassemble";
        case AnalyzeState:         return "analyze";
        case ExtendedAnalyzeState: return "extended_analyze";
    }
    return "unknown";
}

inline const char* EnumToString(BNAnalysisSkipReason reason) {
    switch (reason) {
        case NoSkipReason:                            return "none";
        case AlwaysSkipReason:                        return "always";
        case ExceedFunctionSizeSkipReason:            return "exceed_size";
        case ExceedFunctionAnalysisTimeSkipReason:    return "exceed_time";
        case ExceedFunctionUpdateCountSkipReason:     return "exceed_updates";
        case NewAutoFunctionAnalysisSuppressedReason: return "new_auto_suppressed";
        case BasicAnalysisSkipReason:                 return "basic_analysis";
        case IntermediateAnalysisSkipReason:          return "intermediate_analysis";
        case AnalysisPipelineSuspendedReason:         return "pipeline_suspended";
    }
    return "unknown";
}

inline const char* EnumToString(BNMetadataType type) {
    switch (type) {
        case InvalidDataType:         return "invalid";
        case BooleanDataType:         return "boolean";
        case StringDataType:          return "string";
        case UnsignedIntegerDataType: return "unsigned_integer";
        case SignedIntegerDataType:   return "signed_integer";
        case DoubleDataType:          return "double";
        case RawDataType:             return "raw";
        case KeyValueDataType:        return "key_value";
        case ArrayDataType:           return "array";
    }
    return "unknown";
}

inline const char* EnumToString(BNMemberAccess access) {
    switch (access) {
        case NoAccess:        return "none";
        case PrivateAccess:   return "private";
        case ProtectedAccess: return "protected";
        case PublicAccess:    return "public";
    }
    return "unknown";
}

inline const char* EnumToString(BNEndianness endianness) {
    switch (endianness) {
        case LittleEndian: return "little";
        case BigEndian:    return "big";
    }
    return "unknown";
}

inline const char* EnumToString(BNImplicitRegisterExtend extend) {
    switch (extend) {
        case NoExtend:              return "none";
        case ZeroExtendToFullWidth: return "zero";
        case SignExtendToFullWidth: return "sign";
    }
    return "unknown";
}

inline const char* EnumToString(BNFlagRole role) {
    switch (role) {
        case SpecialFlagRole:                   return "special";
        case ZeroFlagRole:                      return "zero";
        case PositiveSignFlagRole:              return "pos_sign";
        case NegativeSignFlagRole:              return "neg_sign";
        case CarryFlagRole:                     return "carry";
        case OverflowFlagRole:                  return "overflow";
        case HalfCarryFlagRole:                 return "half_carry";
        case EvenParityFlagRole:                return "even_parity";
        case OddParityFlagRole:                 return "odd_parity";
        case OrderedFlagRole:                   return "ordered";
        case UnorderedFlagRole:                 return "unordered";
        case CarryFlagWithInvertedSubtractRole: return "carry_inv_sub";
    }
    return "unknown";
}

inline const char* EnumToString(BNSymbolBinding binding) {
    switch (binding) {
        case NoBinding:     return "none";
        case LocalBinding:  return "local";
        case GlobalBinding: return "global";
        case WeakBinding:   return "weak";
    }
    return "unknown";
}

inline const char* EnumToString(BNVariableSourceType source) {
    switch (source) {
        case StackVariableSourceType:    return "local";
        case RegisterVariableSourceType: return "register";
        case FlagVariableSourceType:     return "flag";
    }
    return "unknown";
}

inline const char* EnumToString(BNTypeReferenceType ref) {
    // Qualify every enumerator explicitly: the BinaryNinja::ReferenceType
    // enum in binaryninjaapi.h:4983 uses the same unqualified names
    // (DirectTypeReferenceType, IndirectTypeReferenceType, ...) and
    // resolving through the `BN` C enum type is the only way to pick
    // the right overload here.
    switch (ref) {
        case ::DirectTypeReferenceType:   return "direct";
        case ::IndirectTypeReferenceType: return "indirect";
        case ::UnknownTypeReferenceType:  return "unknown";
    }
    return "unknown";
}

// BNSettingsScope is a BN_OPTIONS (bitmask) enum but every caller in the
// binding treats it as a single scope selector. The short vocabulary
// mirrors the Python SettingsScope member names with the "Settings"
// prefix and "Scope" suffix stripped. Unknown/composite values fall
// back to "unknown" - the R8 validation script covers the round-trip.
inline const char* EnumToString(BNSettingsScope scope) {
    switch (scope) {
        case SettingsInvalidScope:  return "invalid";
        case SettingsAutoScope:     return "auto";
        case SettingsDefaultScope:  return "default";
        case SettingsUserScope:     return "user";
        case SettingsProjectScope:  return "project";
        case SettingsResourceScope: return "resource";
    }
    return "unknown";
}

// Note: the C++ TagType::Type typedef and BN core's BNTagTypeType are
// the same enum; TagType::GetType() returns BNTagTypeType. The R7
// retrofit moves the previously hand-rolled switch in tag.cpp onto
// this helper.
inline const char* EnumToString(BNTagTypeType type) {
    switch (type) {
        case UserTagType:         return "user";
        case NotificationTagType: return "notification";
        case BookmarksTagType:    return "bookmarks";
    }
    return "unknown";
}

// Dual-accept reverse lookup for every enum that has an EnumToString
// helper. Each specialization accepts BOTH the short canonical Lua
// string produced by EnumToString (e.g. "unconditional", "Function",
// "ro_data", "idle", "basic_analysis") AND the verbatim C/Python
// enumerator name (e.g. "UnconditionalBranch", "FunctionSymbol",
// "ReadOnlyCodeSectionSemantics", "IdleState", "BasicAnalysisSkipReason").
// Matches are case-sensitive and return std::nullopt on miss.
//
// See docs/api-reference.md "Enum vocabulary" for the full table.
template <typename E>
std::optional<E> EnumFromString(const std::string& s);

template <>
inline std::optional<BNBranchType> EnumFromString<BNBranchType>(
    const std::string& s) {
    if (s == "unconditional" || s == "UnconditionalBranch")
        return UnconditionalBranch;
    if (s == "false" || s == "FalseBranch") return FalseBranch;
    if (s == "true"  || s == "TrueBranch")  return TrueBranch;
    if (s == "call"  || s == "CallDestination") return CallDestination;
    if (s == "return" || s == "FunctionReturn") return FunctionReturn;
    if (s == "syscall" || s == "SystemCall") return SystemCall;
    if (s == "indirect" || s == "IndirectBranch") return IndirectBranch;
    if (s == "exception" || s == "ExceptionBranch") return ExceptionBranch;
    if (s == "unresolved" || s == "UnresolvedBranch") return UnresolvedBranch;
    if (s == "user_defined" || s == "UserDefinedBranch")
        return UserDefinedBranch;
    return std::nullopt;
}

template <>
inline std::optional<BNSymbolType> EnumFromString<BNSymbolType>(
    const std::string& s) {
    if (s == "Function" || s == "FunctionSymbol") return FunctionSymbol;
    if (s == "ImportAddress" || s == "ImportAddressSymbol")
        return ImportAddressSymbol;
    if (s == "ImportedFunction" || s == "ImportedFunctionSymbol")
        return ImportedFunctionSymbol;
    if (s == "Data" || s == "DataSymbol") return DataSymbol;
    if (s == "ImportedData" || s == "ImportedDataSymbol")
        return ImportedDataSymbol;
    if (s == "External" || s == "ExternalSymbol") return ExternalSymbol;
    if (s == "LibraryFunction" || s == "LibraryFunctionSymbol")
        return LibraryFunctionSymbol;
    if (s == "SymbolicFunction" || s == "SymbolicFunctionSymbol")
        return SymbolicFunctionSymbol;
    if (s == "LocalLabel" || s == "LocalLabelSymbol") return LocalLabelSymbol;
    return std::nullopt;
}

template <>
inline std::optional<BNSectionSemantics>
EnumFromString<BNSectionSemantics>(const std::string& s) {
    if (s == "default" || s == "DefaultSectionSemantics")
        return DefaultSectionSemantics;
    if (s == "code" || s == "ReadOnlyCodeSectionSemantics")
        return ReadOnlyCodeSectionSemantics;
    if (s == "ro_data" || s == "ReadOnlyDataSectionSemantics")
        return ReadOnlyDataSectionSemantics;
    if (s == "rw_data" || s == "ReadWriteDataSectionSemantics")
        return ReadWriteDataSectionSemantics;
    if (s == "external" || s == "ExternalSectionSemantics")
        return ExternalSectionSemantics;
    return std::nullopt;
}

template <>
inline std::optional<BNTypeClass> EnumFromString<BNTypeClass>(
    const std::string& s) {
    if (s == "Void" || s == "VoidTypeClass") return VoidTypeClass;
    if (s == "Bool" || s == "BoolTypeClass") return BoolTypeClass;
    if (s == "Integer" || s == "IntegerTypeClass") return IntegerTypeClass;
    if (s == "Float" || s == "FloatTypeClass") return FloatTypeClass;
    if (s == "Structure" || s == "StructureTypeClass")
        return StructureTypeClass;
    if (s == "Enumeration" || s == "EnumerationTypeClass")
        return EnumerationTypeClass;
    if (s == "Pointer" || s == "PointerTypeClass") return PointerTypeClass;
    if (s == "Array" || s == "ArrayTypeClass") return ArrayTypeClass;
    if (s == "Function" || s == "FunctionTypeClass") return FunctionTypeClass;
    if (s == "VarArgs" || s == "VarArgsTypeClass") return VarArgsTypeClass;
    if (s == "Value" || s == "ValueTypeClass") return ValueTypeClass;
    if (s == "NamedTypeReference" || s == "NamedTypeReferenceClass")
        return NamedTypeReferenceClass;
    if (s == "WideChar" || s == "WideCharTypeClass") return WideCharTypeClass;
    return std::nullopt;
}

template <>
inline std::optional<BNAnalysisState> EnumFromString<BNAnalysisState>(
    const std::string& s) {
    if (s == "initial" || s == "InitialState") return InitialState;
    if (s == "hold" || s == "HoldState") return HoldState;
    if (s == "idle" || s == "IdleState") return IdleState;
    if (s == "discovery" || s == "DiscoveryState") return DiscoveryState;
    if (s == "disassemble" || s == "DisassembleState")
        return DisassembleState;
    if (s == "analyze" || s == "AnalyzeState") return AnalyzeState;
    if (s == "extended_analyze" || s == "ExtendedAnalyzeState")
        return ExtendedAnalyzeState;
    return std::nullopt;
}

template <>
inline std::optional<BNAnalysisSkipReason>
EnumFromString<BNAnalysisSkipReason>(const std::string& s) {
    if (s == "none" || s == "NoSkipReason") return NoSkipReason;
    if (s == "always" || s == "AlwaysSkipReason") return AlwaysSkipReason;
    if (s == "exceed_size" || s == "ExceedFunctionSizeSkipReason")
        return ExceedFunctionSizeSkipReason;
    if (s == "exceed_time" || s == "ExceedFunctionAnalysisTimeSkipReason")
        return ExceedFunctionAnalysisTimeSkipReason;
    if (s == "exceed_updates" || s == "ExceedFunctionUpdateCountSkipReason")
        return ExceedFunctionUpdateCountSkipReason;
    if (s == "new_auto_suppressed" ||
        s == "NewAutoFunctionAnalysisSuppressedReason")
        return NewAutoFunctionAnalysisSuppressedReason;
    if (s == "basic_analysis" || s == "BasicAnalysisSkipReason")
        return BasicAnalysisSkipReason;
    if (s == "intermediate_analysis" ||
        s == "IntermediateAnalysisSkipReason")
        return IntermediateAnalysisSkipReason;
    if (s == "pipeline_suspended" || s == "AnalysisPipelineSuspendedReason")
        return AnalysisPipelineSuspendedReason;
    return std::nullopt;
}

template <>
inline std::optional<BNMetadataType> EnumFromString<BNMetadataType>(
    const std::string& s) {
    if (s == "invalid" || s == "InvalidDataType") return InvalidDataType;
    if (s == "boolean" || s == "BooleanDataType") return BooleanDataType;
    if (s == "string" || s == "StringDataType") return StringDataType;
    if (s == "unsigned_integer" || s == "UnsignedIntegerDataType")
        return UnsignedIntegerDataType;
    if (s == "signed_integer" || s == "SignedIntegerDataType")
        return SignedIntegerDataType;
    if (s == "double" || s == "DoubleDataType") return DoubleDataType;
    if (s == "raw" || s == "RawDataType") return RawDataType;
    if (s == "key_value" || s == "KeyValueDataType") return KeyValueDataType;
    if (s == "array" || s == "ArrayDataType") return ArrayDataType;
    return std::nullopt;
}

template <>
inline std::optional<BNMemberAccess> EnumFromString<BNMemberAccess>(
    const std::string& s) {
    if (s == "none" || s == "NoAccess") return NoAccess;
    if (s == "private" || s == "PrivateAccess") return PrivateAccess;
    if (s == "protected" || s == "ProtectedAccess") return ProtectedAccess;
    if (s == "public" || s == "PublicAccess") return PublicAccess;
    return std::nullopt;
}

template <>
inline std::optional<BNEndianness> EnumFromString<BNEndianness>(
    const std::string& s) {
    if (s == "little" || s == "LittleEndian") return LittleEndian;
    if (s == "big" || s == "BigEndian") return BigEndian;
    return std::nullopt;
}

template <>
inline std::optional<BNImplicitRegisterExtend>
EnumFromString<BNImplicitRegisterExtend>(const std::string& s) {
    if (s == "none" || s == "NoExtend") return NoExtend;
    if (s == "zero" || s == "ZeroExtendToFullWidth")
        return ZeroExtendToFullWidth;
    if (s == "sign" || s == "SignExtendToFullWidth")
        return SignExtendToFullWidth;
    return std::nullopt;
}

template <>
inline std::optional<BNFlagRole> EnumFromString<BNFlagRole>(
    const std::string& s) {
    if (s == "special" || s == "SpecialFlagRole") return SpecialFlagRole;
    if (s == "zero" || s == "ZeroFlagRole") return ZeroFlagRole;
    if (s == "pos_sign" || s == "PositiveSignFlagRole")
        return PositiveSignFlagRole;
    if (s == "neg_sign" || s == "NegativeSignFlagRole")
        return NegativeSignFlagRole;
    if (s == "carry" || s == "CarryFlagRole") return CarryFlagRole;
    if (s == "overflow" || s == "OverflowFlagRole") return OverflowFlagRole;
    if (s == "half_carry" || s == "HalfCarryFlagRole")
        return HalfCarryFlagRole;
    if (s == "even_parity" || s == "EvenParityFlagRole")
        return EvenParityFlagRole;
    if (s == "odd_parity" || s == "OddParityFlagRole")
        return OddParityFlagRole;
    if (s == "ordered" || s == "OrderedFlagRole") return OrderedFlagRole;
    if (s == "unordered" || s == "UnorderedFlagRole")
        return UnorderedFlagRole;
    if (s == "carry_inv_sub" || s == "CarryFlagWithInvertedSubtractRole")
        return CarryFlagWithInvertedSubtractRole;
    return std::nullopt;
}

template <>
inline std::optional<BNSymbolBinding> EnumFromString<BNSymbolBinding>(
    const std::string& s) {
    if (s == "none" || s == "NoBinding") return NoBinding;
    if (s == "local" || s == "LocalBinding") return LocalBinding;
    if (s == "global" || s == "GlobalBinding") return GlobalBinding;
    if (s == "weak" || s == "WeakBinding") return WeakBinding;
    return std::nullopt;
}

template <>
inline std::optional<BNVariableSourceType>
EnumFromString<BNVariableSourceType>(const std::string& s) {
    if (s == "local" || s == "StackVariableSourceType")
        return StackVariableSourceType;
    if (s == "register" || s == "RegisterVariableSourceType")
        return RegisterVariableSourceType;
    if (s == "flag" || s == "FlagVariableSourceType")
        return FlagVariableSourceType;
    return std::nullopt;
}

template <>
inline std::optional<BNTypeReferenceType>
EnumFromString<BNTypeReferenceType>(const std::string& s) {
    if (s == "direct" || s == "DirectTypeReferenceType")
        return ::DirectTypeReferenceType;
    if (s == "indirect" || s == "IndirectTypeReferenceType")
        return ::IndirectTypeReferenceType;
    if (s == "unknown" || s == "UnknownTypeReferenceType")
        return ::UnknownTypeReferenceType;
    return std::nullopt;
}

template <>
inline std::optional<BNSettingsScope> EnumFromString<BNSettingsScope>(
    const std::string& s) {
    if (s == "invalid" || s == "SettingsInvalidScope")
        return SettingsInvalidScope;
    if (s == "auto" || s == "SettingsAutoScope") return SettingsAutoScope;
    if (s == "default" || s == "SettingsDefaultScope")
        return SettingsDefaultScope;
    if (s == "user" || s == "SettingsUserScope") return SettingsUserScope;
    if (s == "project" || s == "SettingsProjectScope")
        return SettingsProjectScope;
    if (s == "resource" || s == "SettingsResourceScope")
        return SettingsResourceScope;
    return std::nullopt;
}

template <>
inline std::optional<BNTagTypeType> EnumFromString<BNTagTypeType>(
    const std::string& s) {
    if (s == "user" || s == "UserTagType") return UserTagType;
    if (s == "notification" || s == "NotificationTagType")
        return NotificationTagType;
    if (s == "bookmarks" || s == "BookmarksTagType")
        return BookmarksTagType;
    return std::nullopt;
}

// Build a Lua table describing a ReferenceSource. Shape matches the
// Python API's binaryninja.ReferenceSource roughly: address as a
// HexAddress, func as the owning Function (absent when null), and arch
// as the architecture's name (absent when null).
inline sol::table ReferenceSourceToTable(sol::state_view lua,
                                          const ReferenceSource& ref) {
    sol::table t = lua.create_table(0, 3);
    t["address"] = HexAddress(ref.addr);
    if (ref.func) {
        t["func"] = ref.func;
    }
    if (ref.arch) {
        t["arch"] = ref.arch->GetName();
    }
    return t;
}

inline sol::table ReferenceSourcesToTable(
    sol::this_state ts, const std::vector<ReferenceSource>& refs) {
    sol::state_view lua(ts);
    sol::table result = lua.create_table(static_cast<int>(refs.size()), 0);
    for (size_t i = 0; i < refs.size(); ++i) {
        result[i + 1] = ReferenceSourceToTable(lua, refs[i]);
    }
    return result;
}

// Metadata <-> Lua value marshalling. Both directions are shared by the
// BinaryView and Function metadata accessors so the translation logic
// lives in one place instead of being copy-pasted into each usertype.
//
// MetadataToLua reads a BNMetadata* and returns a Lua-side representation
// without taking ownership of the metadata object - the caller is still
// responsible for freeing it. The read side dispatches on
// BNMetadataGetType and covers every BNMetadataType variant (Boolean,
// String, Signed/UnsignedInteger, Double, Raw, KeyValue, Array).
// KeyValue and Array recursively decode their contents. There is no
// BNMetadataGetJsonString fallback - an unknown BNMetadataType returns
// nil.
//
// MetadataFromLua creates a fresh BNMetadata* from a Lua value. The
// caller takes ownership and must call BNFreeMetadata on the returned
// pointer. Supported Lua input types:
//   - bool                        -> BooleanDataType
//   - string                      -> StringDataType
//   - number (integer-valued)     -> Signed/UnsignedIntegerDataType
//   - number (non-integer)        -> DoubleDataType
//   - 1-indexed sequential table  -> ArrayDataType (recursive)
//   - string-keyed table          -> KeyValueDataType (recursive)
// Unsupported values return nullptr. Integer encoding defaults to
// signed (lua_Integer is signed); pass prefer_unsigned=true to force
// BNCreateMetadataUnsignedIntegerData for the top-level value and all
// recursively-encoded children. Lua strings always encode as
// StringData - there is no encode path for RawDataType since Lua has
// no distinct binary-blob type. The read path still decodes Raw
// metadata produced by other writers.
sol::object MetadataToLua(sol::state_view lua, BNMetadata* md);
BNMetadata* MetadataFromLua(sol::object value, bool prefer_unsigned = false);

// Extract a uint64_t address from a Lua-side value that may be a
// HexAddress, integer, or floating-point number. Returns std::nullopt if
// the value is of no recognisable numeric type. Lets bindings accept both
// raw integers and HexAddress usertypes interchangeably.
inline std::optional<uint64_t> AsAddress(sol::object obj) {
    if (obj.is<HexAddress>()) {
        return obj.as<HexAddress>().value;
    }
    if (obj.is<uint64_t>()) {
        return obj.as<uint64_t>();
    }
    if (obj.is<int64_t>()) {
        return static_cast<uint64_t>(obj.as<int64_t>());
    }
    if (obj.is<double>()) {
        return static_cast<uint64_t>(obj.as<double>());
    }
    return std::nullopt;
}

// InstructionWrapper - represents a single disassembled instruction
// Stores token data with analysis capabilities
class InstructionWrapper {
public:
    uint64_t address;
    std::string mnemonic;
    std::vector<BNInstructionTextToken> tokens;
    Ref<BinaryView> view;
    Ref<Architecture> arch;

    InstructionWrapper(uint64_t addr, const std::vector<BNInstructionTextToken>& inst_tokens,
                      Ref<BinaryView> bv, Ref<Architecture> architecture);
    InstructionWrapper(const InstructionWrapper& other);
    ~InstructionWrapper();

    // Methods
    std::string GetMnemonic() const;
    size_t GetLength() const;
    std::vector<uint8_t> GetBytes() const;
    std::vector<std::string> GetOperands() const;
    std::vector<uint64_t> GetCodeReferences() const;
    std::vector<uint64_t> GetDataReferences() const;
    std::string GetText() const;
};

// VariableWrapper - represents a function variable with lazy loading
class VariableWrapper {
public:
    BNVariable bnVar;
    Ref<Function> function;
    mutable std::string cachedName;
    mutable std::string cachedTypeName;
    mutable bool nameResolved;
    mutable bool typeResolved;

    VariableWrapper(const BNVariable& var, Ref<Function> func);
    VariableWrapper(const VariableWrapper& other);
    ~VariableWrapper();

    // Lazy loading methods
    void ResolveName() const;
    void ResolveType() const;

    // Accessors
    std::string GetSourceTypeString() const;
    std::string GetName() const;
    std::string GetTypeName() const;
};

// DataVariableWrapper - wraps a data variable with BinaryView context
class DataVariableWrapper {
public:
    uint64_t address;
    Ref<BinaryView> view;
    Confidence<Ref<Type>> type;
    bool autoDiscovered;

    DataVariableWrapper(uint64_t addr, Ref<BinaryView> bv,
                       const Confidence<Ref<Type>>& t, bool autoDisco);
    DataVariableWrapper(const DataVariableWrapper& other);
    ~DataVariableWrapper();

    // Accessors
    std::string GetTypeName() const;
    std::string GetName() const;
};

} // namespace BinjaLua

// Note: Ref<T> handling with sol2 is done by registering Ref<T> directly
// as the usertype. Sol2 will handle copy/move semantics for types it manages.
