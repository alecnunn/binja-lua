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

inline const char* EnumToString(BNSymbolBinding binding) {
    switch (binding) {
        case NoBinding:     return "none";
        case LocalBinding:  return "local";
        case GlobalBinding: return "global";
        case WeakBinding:   return "weak";
    }
    return "unknown";
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
