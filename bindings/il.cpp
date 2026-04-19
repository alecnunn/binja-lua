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

        // Replaces the R9.0 stub that returned {index = N}. Returns
        // a BinaryNinja.MLILInstruction usertype registered by
        // RegisterMLILInstructionBindings (see this file). Index
        // out of range surfaces as nil - same contract as Python's
        // MediumLevelILFunction[i] subscript, which raises IndexError;
        // Lua scripts check for nil.
        "instruction_at",
        [](sol::this_state ts, MediumLevelILFunction& il, size_t index)
            -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) {
                return sol::make_object(lua, sol::lua_nil_t{});
            }
            return sol::make_object(lua, il[index]);
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

        // Replaces the R9.0 stub that returned {index = N}. Returns
        // a BinaryNinja.HLILInstruction usertype registered by
        // RegisterHLILInstructionBindings (see this file). Same
        // out-of-range nil contract as LLIL/MLIL.
        "instruction_at",
        [](sol::this_state ts, HighLevelILFunction& il, size_t index)
            -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) {
                return sol::make_object(lua, sol::lua_nil_t{});
            }
            return sol::make_object(lua, il[index]);
        },

        // HLIL-unique: expose the AST root expression. HLIL functions
        // carry a single AST root (unlike LLIL/MLIL's flat instruction
        // list), so scripts that want to walk the entire function's
        // tree usually start here. Parallel to the C++ surface at
        // binaryninjaapi.h:15917. Python intentionally does not expose
        // this (users walk via ancestors() / get_expr per
        // py-researcher-2's #34 refresh), but the C++ entry saves
        // scripts from ancestor-walks.
        "root",
        [](HighLevelILFunction& il) -> HighLevelILInstruction {
            return il.GetRootExpr();
        },

        // Flat expression-index accessor. Parallel to Python
        // HighLevelILFunction.get_expr at python/highlevelil.py:2940.
        // `as_ast` toggles between expression-context and AST-walking
        // views; defaults to true matching the BN C++ default.
        "get_expr",
        [](sol::this_state ts, HighLevelILFunction& il, size_t expr_idx,
           sol::optional<bool> as_ast) -> sol::object {
            sol::state_view lua(ts);
            if (expr_idx >= il.GetExprCount()) {
                return sol::make_object(lua, sol::lua_nil_t{});
            }
            bool full = as_ast.value_or(true);
            return sol::make_object(lua, il.GetExpr(expr_idx, full));
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

        // SSA cross-form. Bound as sol::property so Lua scripts can
        // dot-access them as read-only attributes (instr.ssa_form)
        // matching Python's @property shape at
        // python/lowlevelil.py. No sol::this_state here so the sol2
        // 3.3.0 MSVC property+this_state landmine does not apply.
        "ssa_form", sol::property(
            [](const LowLevelILInstruction& i) -> LowLevelILInstruction {
                return i.GetSSAForm();
            }),
        "non_ssa_form", sol::property(
            [](const LowLevelILInstruction& i) -> LowLevelILInstruction {
                return i.GetNonSSAForm();
            }),
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
        // LLILFunction:get_text(i). Bound as sol::property so Lua
        // scripts read `instr.text` as a string (the pre-fix plain
        // method form returned the bound Lua function on dot access,
        // breaking 13_llil.lua's is_string assertion). No
        // sol::this_state required, so the sol2 3.3.0 MSVC
        // property+this_state landmine does not apply.
        "text", sol::property(
            [](const LowLevelILInstruction& i) -> std::string {
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
            }),

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

void RegisterMLILInstructionBindings(sol::state_view lua,
                                      Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering MLILInstruction bindings");

    // Second value-usertype in the plugin after LLILInstruction.
    // sol2 stores MediumLevelILInstruction by value; the internal
    // Ref<MediumLevelILFunction> pins the owning IL function for the
    // wrapper's lifetime (see top-of-file note for the LLIL analog).
    //
    // sol2 3.3.0 + MSVC HARD RULE: same as LLIL. Projection helpers
    // (operands / detailed_operands / prefix_operands / traverse)
    // bind as METHODS with sol::this_state as the first parameter,
    // not as sol::property wrappers. SSA cross-form accessors
    // (ssa_form / non_ssa_form) and scalar fields stay as
    // sol::property because they do not need the Lua state.
    lua.new_usertype<MediumLevelILInstruction>(MLIL_INSTRUCTION_METATABLE,
        sol::no_constructor,

        // Scalar fields from the BN struct base.
        "size", &MediumLevelILInstruction::size,
        "expr_index", &MediumLevelILInstruction::exprIndex,
        "instr_index", &MediumLevelILInstruction::instructionIndex,
        "source_operand", &MediumLevelILInstruction::sourceOperand,
        "attributes", &MediumLevelILInstruction::attributes,

        // Address wrapped in HexAddress for consistent printing.
        "address", sol::property(
            [](const MediumLevelILInstruction& i) -> HexAddress {
                return HexAddress(i.address);
            }),

        // Short canonical lowercase-underscore opcode name. Python
        // parity: MLIL_SET_VAR -> "set_var".
        "operation", sol::property(
            [](const MediumLevelILInstruction& i) -> std::string {
                return std::string(EnumToString(i.operation));
            }),

        // Owning IL function as an existing Ref<T> usertype. Same
        // `il_function` naming as LLIL: `function` is a Lua reserved
        // word and cannot be used as an identifier in
        // `instr.function`-style access.
        "il_function", sol::property(
            [](const MediumLevelILInstruction& i)
                -> Ref<MediumLevelILFunction> { return i.function; }),

        // Operand-projection accessors. sol::this_state-carrying
        // methods, so bound as plain methods per the hard rule.
        "operands", &BuildMLILOperandsTable,
        "detailed_operands", &BuildMLILDetailedOperandsTable,
        "prefix_operands", &BuildMLILPrefixOperandsTable,

        // SSA cross-form. No sol::this_state, so sol::property is
        // safe. `ssa_instr_index` / `ssa_expr_index` are bound as
        // plain methods with zero args; sol2 auto-invokes zero-arg
        // methods on property access so Lua scripts can write
        // `instr:ssa_expr_index()` or (via auto-invoke) plain access.
        "ssa_form", sol::property(
            [](const MediumLevelILInstruction& i)
                -> MediumLevelILInstruction {
                return i.GetSSAForm();
            }),
        "non_ssa_form", sol::property(
            [](const MediumLevelILInstruction& i)
                -> MediumLevelILInstruction {
                return i.GetNonSSAForm();
            }),
        "ssa_instr_index",
        [](const MediumLevelILInstruction& i) -> size_t {
            return i.GetSSAInstructionIndex();
        },
        "ssa_expr_index",
        [](const MediumLevelILInstruction& i) -> size_t {
            return i.GetSSAExprIndex();
        },

        // Rendered instruction text, same path as
        // MLILFunction:get_text(i). sol::property-wrapped so
        // `instr.text` returns a string directly.
        "text", sol::property(
            [](const MediumLevelILInstruction& i) -> std::string {
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
            }),

        // Depth-first pre-order walker. Python parity:
        // MediumLevelILInstruction.traverse.
        "traverse", &TraverseMLILInstruction,

        // Metamethods.
        sol::meta_function::equal_to,
        [](const MediumLevelILInstruction& a,
           const MediumLevelILInstruction& b) -> bool {
            return a.function.GetPtr() == b.function.GetPtr() &&
                   a.exprIndex == b.exprIndex;
        },

        sol::meta_function::to_string,
        [](const MediumLevelILInstruction& i) -> std::string {
            return fmt::format("<MLILInstruction {} @0x{:x}>",
                               EnumToString(i.operation), i.address);
        }
    );

    if (logger) logger->LogDebug("MLILInstruction bindings registered");
}

void RegisterHLILInstructionBindings(sol::state_view lua,
                                      Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering HLILInstruction bindings");

    // Third value-usertype in the plugin after LLILInstruction and
    // MLILInstruction. sol2 stores HighLevelILInstruction by value;
    // the internal Ref<HighLevelILFunction> pins the owning IL
    // function. See bindings/il_operand_conv.cpp for the family-wide
    // value-usertype note.
    //
    // sol2 3.3.0 + MSVC HARD RULE: projection + tree-navigation
    // helpers that need sol::this_state bind as METHODS, never as
    // sol::property. The same rule applies to MLIL/LLIL.
    //
    // Property set differs from LLIL/MLIL per spec section 13.5:
    //   - No flags, instr_index, has_mlil, has_mapped_mlil,
    //     ssa_instr_index (HLIL concepts differ).
    //   - Adds parent (Ref<HLILInstruction> or nil), as_ast / ast /
    //     non_ast booleans, :children() / :ancestors() tree-nav
    //     methods, .mlil / :mlils() HLIL->MLIL cross-references.
    //   - No direct .llil: the two-hop chain is intentional per the
    //     Python policy at python/mediumlevelil.py:697.
    lua.new_usertype<HighLevelILInstruction>(HLIL_INSTRUCTION_METATABLE,
        sol::no_constructor,

        // Scalar fields from the BN struct base.
        "size", &HighLevelILInstruction::size,
        "expr_index", &HighLevelILInstruction::exprIndex,
        "source_operand", &HighLevelILInstruction::sourceOperand,
        "attributes", &HighLevelILInstruction::attributes,

        // Address wrapped in HexAddress for consistent printing.
        "address", sol::property(
            [](const HighLevelILInstruction& i) -> HexAddress {
                return HexAddress(i.address);
            }),

        // Short canonical lowercase-underscore opcode name. Python
        // parity: HLIL_ASSIGN -> "assign".
        "operation", sol::property(
            [](const HighLevelILInstruction& i) -> std::string {
                return std::string(EnumToString(i.operation));
            }),

        // Owning IL function as an existing Ref<T> usertype. Same
        // `il_function` naming convention as LLIL/MLIL (dodges the
        // Lua `function` reserved-word collision).
        "il_function", sol::property(
            [](const HighLevelILInstruction& i)
                -> Ref<HighLevelILFunction> { return i.function; }),

        // AST / non-AST context fields. Scalar booleans and
        // scalar-returning projections; no sol::this_state needed.
        "as_ast", sol::property(
            [](const HighLevelILInstruction& i) -> bool {
                return i.ast;
            }),
        "ast", sol::property(
            [](const HighLevelILInstruction& i)
                -> HighLevelILInstruction { return i.AsAST(); }),
        "non_ast", sol::property(
            [](const HighLevelILInstruction& i)
                -> HighLevelILInstruction { return i.AsNonAST(); }),

        // Tree-navigation scalar property. sol2 marshals
        // std::optional<T> as T or nil automatically, so we can keep
        // this as a sol::property without triggering the MSVC 3.3.0
        // landmine (never combine sol::property with sol::this_state).
        // Returns nil when HasParent() is false (root / detached).
        "parent", sol::property(
            [](const HighLevelILInstruction& i)
                -> std::optional<HighLevelILInstruction> {
                if (!i.HasParent()) return std::nullopt;
                return i.GetParent();
            }),

        // Cross-references to MLIL. HLIL.mlil returns an
        // MLILInstruction usertype when HasMediumLevelIL() is true;
        // nil otherwise. HLIL has no direct .llil accessor per spec
        // section 13.5; scripts walk via hlil.mlil.low_level_il.
        // std::optional marshalling keeps this off the sol::property +
        // sol::this_state landmine.
        "mlil", sol::property(
            [](const HighLevelILInstruction& i)
                -> std::optional<MediumLevelILInstruction> {
                if (!i.HasMediumLevelIL()) return std::nullopt;
                return i.GetMediumLevelIL();
            }),

        // Operand-projection accessors + tree-nav methods.
        // sol::this_state-carrying methods, so bound as plain methods
        // per the hard rule.
        "operands", &BuildHLILOperandsTable,
        "detailed_operands", &BuildHLILDetailedOperandsTable,
        "prefix_operands", &BuildHLILPrefixOperandsTable,
        "traverse", &TraverseHLILInstruction,
        "children", &GetHLILChildren,
        "ancestors", &GetHLILAncestors,

        // Multi-result cross-references to MLIL. Python
        // HLILInstruction.mlils at python/highlevelil.py:578 returns
        // the full set of MLIL expressions that map back to this HLIL
        // expression; scripts use it to cover MLIL expansion fan-out
        // that the scalar .mlil accessor hides. We walk
        // HighLevelILFunction.GetMediumLevelILExprIndexes and
        // materialize each MLIL expression via GetExpr on the owning
        // MLIL function.
        "mlils",
        [](sol::this_state ts, const HighLevelILInstruction& i)
            -> sol::table {
            sol::state_view lua(ts);
            sol::table out = lua.create_table();
            if (!i.function) return out;
            std::set<size_t> idxs =
                i.function->GetMediumLevelILExprIndexes(i.exprIndex);
            Ref<MediumLevelILFunction> mlil_fn =
                i.function->GetMediumLevelIL();
            if (!mlil_fn) return out;
            int out_idx = 1;
            for (size_t mlil_expr : idxs) {
                out[out_idx++] = mlil_fn->GetExpr(mlil_expr);
            }
            return out;
        },

        // SSA cross-form. No sol::this_state, so sol::property is
        // safe. ssa_expr_index bound as plain method with zero args;
        // sol2 auto-invokes zero-arg methods on property access.
        "ssa_form", sol::property(
            [](const HighLevelILInstruction& i)
                -> HighLevelILInstruction { return i.GetSSAForm(); }),
        "non_ssa_form", sol::property(
            [](const HighLevelILInstruction& i)
                -> HighLevelILInstruction {
                return i.GetNonSSAForm();
            }),
        "ssa_expr_index",
        [](const HighLevelILInstruction& i) -> size_t {
            return i.GetSSAExprIndex();
        },

        // Rendered instruction text. HLIL text rendering goes through
        // GetExprText on the owning IL function; join the
        // DisassemblyTextLine tokens with newlines the same way the
        // HLIL function-level get_text does. sol::property-wrapped.
        "text", sol::property(
            [](const HighLevelILInstruction& i) -> std::string {
                if (!i.function) return "";
                std::vector<DisassemblyTextLine> lines =
                    i.function->GetExprText(i);
                std::ostringstream ss;
                bool first = true;
                for (const auto& line : lines) {
                    if (!first) ss << "\n";
                    for (const auto& tok : line.tokens) ss << tok.text;
                    first = false;
                }
                return ss.str();
            }),

        // Metamethods.
        sol::meta_function::equal_to,
        [](const HighLevelILInstruction& a,
           const HighLevelILInstruction& b) -> bool {
            return a.function.GetPtr() == b.function.GetPtr() &&
                   a.exprIndex == b.exprIndex;
        },

        sol::meta_function::to_string,
        [](const HighLevelILInstruction& i) -> std::string {
            return fmt::format("<HLILInstruction {} @0x{:x}>",
                               EnumToString(i.operation), i.address);
        }
    );

    if (logger) logger->LogDebug("HLILInstruction bindings registered");
}

} // namespace BinjaLua
