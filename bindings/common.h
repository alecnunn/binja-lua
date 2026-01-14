// Sol2 bindings common header for binja-lua
// Contains metatable names, type traits, and registration declarations

#pragma once

#include "sol_config.h"

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
