// Sol2 IL bindings for binja-lua (LLIL, MLIL, HLIL).
//
// Value-usertype note (R9.1): the LLILInstruction usertype registered
// by RegisterLLILInstructionBindings below is the FIRST non-Ref<T>
// value-semantics usertype in this plugin. Every previous usertype
// wraps a BN Ref<T> via sol2's unique_usertype_traits specialization
// in bindings/sol_config.h. LowLevelILInstruction is a BN value type
// that internally carries a Ref<LowLevelILFunction>; sol2 stores it
// by value. Copies are cheap (pointer + two size_t) and safe. Do NOT
// try to route LLILInstruction through the Ref<T> traits - sol2
// handles value usertypes directly. Operands are projected to plain
// Lua values (primitives, existing BN usertypes, nested instruction
// usertypes, or plain Lua tables) by the helpers in
// bindings/il_operand_conv.cpp. There are no *Operand usertypes;
// see docs/il-metatable-design.md for the rationale.
//
// sol2 3.3.0 + MSVC HARD RULE: NEVER combine sol::property with
// sol::this_state in a lambda. Every accessor that needs the Lua
// state must be registered as a plain method (colon call on the
// Lua side) with sol::this_state as its first PARAMETER. sol2 auto-
// invokes zero-arg methods when accessed as properties, so from Lua
// `instr.operands` works the same as `instr:operands()` - no
// ergonomic cost. Same rule enforced in bindings/architecture.cpp
// line 154, bindings/binaryview.cpp line 45, bindings/function.cpp
// line 137, bindings/flowgraph.cpp line 102, bindings/settings.cpp.

#include "common.h"
#include "il.h"
#include <sstream>

namespace BinjaLua {

void RegisterILBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering IL bindings");

    // LLIL bindings
    lua.new_usertype<LowLevelILFunction>(LLIL_METATABLE,
        sol::no_constructor,

        "instruction_count", sol::property([](LowLevelILFunction& il) -> size_t {
            return il.GetInstructionCount();
        }),

        "basic_block_count", sol::property([](LowLevelILFunction& il) -> size_t {
            return il.GetBasicBlocks().size();
        }),

        "get_function", [](LowLevelILFunction& il) -> Ref<Function> {
            return il.GetFunction();
        },

        // Replaces the R9.0 stub that returned {index = N}. Returns
        // a BinaryNinja.LLILInstruction usertype registered by
        // RegisterLLILInstructionBindings (see this file). Index
        // out of range surfaces as nil - same contract as Python's
        // LowLevelILFunction[i] subscript, which raises IndexError;
        // Lua scripts check for nil.
        "instruction_at",
        [](sol::this_state ts, LowLevelILFunction& il, size_t index)
            -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) {
                return sol::make_object(lua, sol::lua_nil_t{});
            }
            return sol::make_object(lua, il[index]);
        },

        "get_text", [](LowLevelILFunction& il, size_t index) -> std::string {
            if (index >= il.GetInstructionCount()) return "";
            std::vector<InstructionTextToken> tokens;
            Ref<Function> func = il.GetFunction();
            if (func && il.GetInstructionText(func, func->GetArchitecture(), index, tokens)) {
                std::ostringstream ss;
                for (const auto& token : tokens) ss << token.text;
                return ss.str();
            }
            return "LLIL instruction";
        },

        // Create flow graph from LLIL
        "create_graph", [](LowLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraph();
        },

        "create_graph_immediate", [](LowLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraphImmediate();
        },

        sol::meta_function::equal_to, [](LowLevelILFunction& a, LowLevelILFunction& b) -> bool {
            return LowLevelILFunction::GetObject(&a) == LowLevelILFunction::GetObject(&b);
        },

        sol::meta_function::to_string, [](LowLevelILFunction& il) -> std::string {
            return fmt::format("<LLIL: {} instructions>", il.GetInstructionCount());
        }
    );

    // MLIL bindings
    lua.new_usertype<MediumLevelILFunction>(MLIL_METATABLE,
        sol::no_constructor,

        "instruction_count", sol::property([](MediumLevelILFunction& il) -> size_t {
            return il.GetInstructionCount();
        }),

        "basic_block_count", sol::property([](MediumLevelILFunction& il) -> size_t {
            return il.GetBasicBlocks().size();
        }),

        "get_function", [](MediumLevelILFunction& il) -> Ref<Function> {
            return il.GetFunction();
        },

        "instruction_at", [](sol::this_state ts, MediumLevelILFunction& il, size_t index) -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) return sol::nil;
            sol::table result = lua.create_table();
            result["index"] = index;
            return result;
        },

        "get_text", [](MediumLevelILFunction& il, size_t index) -> std::string {
            if (index >= il.GetInstructionCount()) return "";
            std::vector<InstructionTextToken> tokens;
            Ref<Function> func = il.GetFunction();
            if (func && il.GetInstructionText(func, func->GetArchitecture(), index, tokens)) {
                std::ostringstream ss;
                for (const auto& token : tokens) ss << token.text;
                return ss.str();
            }
            return "MLIL instruction";
        },

        // Create flow graph from MLIL
        "create_graph", [](MediumLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraph();
        },

        "create_graph_immediate", [](MediumLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraphImmediate();
        },

        sol::meta_function::equal_to, [](MediumLevelILFunction& a, MediumLevelILFunction& b) -> bool {
            return MediumLevelILFunction::GetObject(&a) == MediumLevelILFunction::GetObject(&b);
        },

        sol::meta_function::to_string, [](MediumLevelILFunction& il) -> std::string {
            return fmt::format("<MLIL: {} instructions>", il.GetInstructionCount());
        }
    );

    // HLIL bindings
    lua.new_usertype<HighLevelILFunction>(HLIL_METATABLE,
        sol::no_constructor,

        "instruction_count", sol::property([](HighLevelILFunction& il) -> size_t {
            return il.GetInstructionCount();
        }),

        "basic_block_count", sol::property([](HighLevelILFunction& il) -> size_t {
            return il.GetBasicBlocks().size();
        }),

        "get_function", [](HighLevelILFunction& il) -> Ref<Function> {
            return il.GetFunction();
        },

        "instruction_at", [](sol::this_state ts, HighLevelILFunction& il, size_t index) -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) return sol::nil;
            sol::table result = lua.create_table();
            result["index"] = index;
            return result;
        },

        "get_text", [](HighLevelILFunction& il, size_t index) -> std::string {
            if (index >= il.GetInstructionCount()) return "";
            auto textLines = il.GetInstructionText(index);
            if (!textLines.empty()) {
                std::ostringstream ss;
                bool first = true;
                for (const auto& line : textLines) {
                    if (!first) ss << "\n";
                    for (const auto& token : line.tokens) ss << token.text;
                    first = false;
                }
                return ss.str();
            }
            return "HLIL instruction";
        },

        // Create flow graph from HLIL
        "create_graph", [](HighLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraph();
        },

        "create_graph_immediate", [](HighLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraphImmediate();
        },

        sol::meta_function::equal_to, [](HighLevelILFunction& a, HighLevelILFunction& b) -> bool {
            return HighLevelILFunction::GetObject(&a) == HighLevelILFunction::GetObject(&b);
        },

        sol::meta_function::to_string, [](HighLevelILFunction& il) -> std::string {
            return fmt::format("<HLIL: {} instructions>", il.GetInstructionCount());
        }
    );

    if (logger) logger->LogDebug("IL bindings registered");
}

void RegisterLLILInstructionBindings(sol::state_view lua,
                                      Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering LLILInstruction bindings");

    // First non-Ref<T> value-usertype in the plugin (see top-of-file
    // note). sol2 stores LowLevelILInstruction by value; the
    // internal Ref<LowLevelILFunction> pins the owning IL function
    // for the wrapper's lifetime.
    lua.new_usertype<LowLevelILInstruction>(LLIL_INSTRUCTION_METATABLE,
        sol::no_constructor,

        // Scalar fields straight out of the struct.
        "size", &LowLevelILInstruction::size,
        "expr_index", &LowLevelILInstruction::exprIndex,
        "instr_index", &LowLevelILInstruction::instructionIndex,
        "source_operand", &LowLevelILInstruction::sourceOperand,
        "flags", &LowLevelILInstruction::flags,
        "attributes", &LowLevelILInstruction::attributes,

        // Address wrapped in HexAddress for consistent printing; same
        // convention as Symbol.address and Function.start_addr.
        "address", sol::property(
            [](const LowLevelILInstruction& i) -> HexAddress {
                return HexAddress(i.address);
            }),

        // Short canonical lowercase-underscore opcode name. Python
        // parity: LLIL_SET_REG -> "set_reg". See
        // docs/il-metatable-design.md section 4.2a.
        "operation", sol::property(
            [](const LowLevelILInstruction& i) -> std::string {
                return std::string(EnumToString(i.operation));
            }),

        // Owning IL function as an existing Ref<T> usertype. Named
        // `il_function` rather than `function` because `function` is
        // a Lua reserved word and cannot be used as an identifier in
        // `instr.function`-style access. Same canonical-name rule as
        // the R3d `start_addr` / `end_addr` / `auto_discovered` /
        // `type_name` / `filename` renames. No alias.
        "il_function", sol::property(
            [](const LowLevelILInstruction& i)
                -> Ref<LowLevelILFunction> { return i.function; }),

        // Operand-projection accessors. Bound as methods (not
        // sol::property) because they take sol::this_state - combining
        // property with this_state crashes sol2 3.3.0 on MSVC. sol2
        // auto-invokes zero-arg methods as properties, so from the Lua
        // side `instr.operands` still works.
        "operands", &BuildLLILOperandsTable,
        "detailed_operands", &BuildLLILDetailedOperandsTable,
        "prefix_operands", &BuildLLILPrefixOperandsTable,

        // SSA cross-form.
        "ssa_form", [](const LowLevelILInstruction& i)
            -> LowLevelILInstruction { return i.GetSSAForm(); },
        "non_ssa_form", [](const LowLevelILInstruction& i)
            -> LowLevelILInstruction { return i.GetNonSSAForm(); },
        "ssa_instr_index",
        [](const LowLevelILInstruction& i) -> size_t {
            return i.GetSSAInstructionIndex();
        },
        "ssa_expr_index",
        [](const LowLevelILInstruction& i) -> size_t {
            return i.GetSSAExprIndex();
        },

        // MLIL cross-layer (available when HasMediumLevelIL).
        "has_mlil", [](const LowLevelILInstruction& i) -> bool {
            return i.HasMediumLevelIL();
        },
        "has_mapped_mlil",
        [](const LowLevelILInstruction& i) -> bool {
            return i.HasMappedMediumLevelIL();
        },

        // Rendered instruction text. Same path as
        // LLILFunction:get_text(i).
        "text", [](const LowLevelILInstruction& i) -> std::string {
            if (!i.function) return "";
            Ref<Function> f = i.function->GetFunction();
            if (!f) return "";
            std::vector<InstructionTextToken> tokens;
            if (i.function->GetInstructionText(
                    f, f->GetArchitecture(),
                    i.instructionIndex, tokens)) {
                std::ostringstream ss;
                for (const auto& t : tokens) ss << t.text;
                return ss.str();
            }
            return "";
        },

        // Depth-first pre-order walker. Matches Python's
        // LowLevelILInstruction.traverse.
        "traverse", &TraverseLLILInstruction,

        // Metamethods.
        sol::meta_function::equal_to,
        [](const LowLevelILInstruction& a,
           const LowLevelILInstruction& b) -> bool {
            return a.function.GetPtr() == b.function.GetPtr() &&
                   a.exprIndex == b.exprIndex;
        },

        sol::meta_function::to_string,
        [](const LowLevelILInstruction& i) -> std::string {
            return fmt::format("<LLILInstruction {} @0x{:x}>",
                               EnumToString(i.operation), i.address);
        }
    );

    if (logger) logger->LogDebug("LLILInstruction bindings registered");
}

} // namespace BinjaLua
