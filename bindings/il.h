// IL binding declarations for binja-lua (LLIL / MLIL / HLIL wave).
//
// R9.1 commit 2a: LLILOperandSpec struct + per-opcode dispatch
// declaration. R9.1 commit 2b: projection helpers + traverse worker.
// R9.2 commit B: MLILOperandSpec + MLIL projection + MLILInstruction
// usertype registration. HLIL sibling (R9.3) will add the analogous
// HLIL declarations.

#pragma once

#include "common.h"

// Full definitions of LowLevelILInstruction / MediumLevelILInstruction
// and the *List container classes. binaryninjaapi.h only forward-
// declares them, so we need the full headers to call GetRawOperandAs*
// methods and to iterate the list containers.
#include "lowlevelilinstruction.h"
#include "mediumlevelilinstruction.h"

#include <cstdint>
#include <vector>

namespace BinjaLua {

// Per-operand spec emitted by scripts/generate_il_tables.py from
// python/lowlevelil.py's per-subclass detailed_operands overrides.
// See docs/il-metatable-design.md section 2c for the type_tag
// vocabulary.
//
// slot_first / slot_last encode the raw-instruction operand slot
// range. Single-slot tags use slot_last == slot_first. Explicit-pair
// SSA tags (reg_ssa, flag_ssa, reg_stack_ssa) use slot_last ==
// slot_first + 1. Single-slot list accessors (expr_list, int_list,
// target_map, reg_stack_adjust, reg_or_flag_list, *_ssa_list,
// reg_or_flag_ssa_list) still use slot_last == slot_first because BN's
// GetRawOperandAs*List consumes the (size, operand_idx) pair from a
// single slot argument internally.
struct LLILOperandSpec {
    const char* name;
    const char* type_tag;
    uint8_t slot_first;
    uint8_t slot_last;
};

// Per-operand spec for MLIL. Field-identical to LLILOperandSpec by
// design (see docs/il-metatable-design.md section 12.5); kept as a
// distinct type so the generator and projection dispatcher are
// compile-time-separate between the families - a future HLIL tag
// that does not exist in MLIL cannot sneak into the LLIL dispatcher
// via accidental struct aliasing. Tag vocabulary extends LLIL's with
// MLIL-specific tags: "var", "var_ssa", "var_list", "var_ssa_list",
// "var_ssa_dest_and_src", "ConstantData" (CamelCase per Python's
// ConstantDataType, per spec section 12.3 gotcha). MLIL drops all
// register-level tags (reg, flag, reg_stack, sem_class, sem_group,
// reg_ssa, flag_ssa, reg_stack_ssa, reg_*_list) since MLIL operates
// on typed variables rather than raw registers.
struct MLILOperandSpec {
    const char* name;
    const char* type_tag;
    uint8_t slot_first;
    uint8_t slot_last;
};

// Declarations for the EnumToString overloads generated in
// bindings/il_enums.inc. The .inc is included inside
// bindings/il_operand_conv.cpp's BinjaLua namespace, so the
// definitions live in that TU; bindings/il.cpp needs to call them to
// render BNLowLevelILOperation and BNLowLevelILFlagCondition in the
// LLILInstruction usertype's `operation` property and `__tostring`
// metamethod. EnumFromString<> specializations follow the same
// pattern and are declared here so future IL-consumer TUs can reach
// them; template specializations in the .inc carry external linkage
// once the declaration is visible.
const char* EnumToString(BNLowLevelILOperation v);
const char* EnumToString(BNLowLevelILFlagCondition v);
// R9.2 addition. Definition lives in il_operand_conv.cpp via the
// regenerated il_enums.inc. No TU references this yet in commit A
// (enums-only); MLILInstruction usertype in commit B will use it.
const char* EnumToString(BNMediumLevelILOperation v);

// Per-opcode dispatch. Returns a reference to a static empty vector
// when the opcode has no detailed_operands override in Python (e.g.
// LLIL_NOP, LLIL_POP, LLIL_NORET, LLIL_SYSCALL, LLIL_BP, LLIL_UNDEF,
// LLIL_UNIMPL). Generated implementation lives in
// bindings/il_operands_table.inc.
const std::vector<LLILOperandSpec>& LLILOperandSpecsForOperation(
    BNLowLevelILOperation op);

// Resolve the owning Architecture for a LowLevelILInstruction, or
// nullptr when the IL function has no attached BN Function (rare; only
// occurs with synthetic IL). Every operand projector that turns a
// register/flag/intrinsic slot into a name string goes through this
// helper and returns nil on null.
Ref<Architecture> ArchFor(const LowLevelILInstruction& instr);

// Project a single operand spec to a Lua value following the type_tag
// vocabulary at docs/il-metatable-design.md section 2c. Returns
// sol::nil on architecture-unavailable / sentinel register id /
// unknown tag. The custom "unknown" tag used by the 7 call-param
// delegation sites (LLIL_CALL_SSA.params etc.) dispatches on
// (spec.slot_first, spec.name) via ProjectUnknownField.
sol::object LLILOperandToLua(sol::state_view lua,
                              const LowLevelILInstruction& instr,
                              const LLILOperandSpec& spec);

// Build the 1-indexed `instr.operands` list: projects each operand
// spec via LLILOperandToLua without the (name, type) wrapping.
sol::table BuildLLILOperandsTable(sol::this_state ts,
                                    const LowLevelILInstruction& instr);

// Build the 1-indexed `instr.detailed_operands` list: each entry is
// {name=..., value=..., type=...} matching Python's detailed_operands.
sol::table BuildLLILDetailedOperandsTable(
    sol::this_state ts, const LowLevelILInstruction& instr);

// Build the prefix-order flattened walk of the expression tree.
// Matches Python LowLevelILInstruction.prefix_operands at
// python/lowlevelil.py:837. Each entry is either a
// {operation=<short>, size=<int>} marker table or a projected operand
// value (primitive, name string, nested instruction, etc.).
sol::table BuildLLILPrefixOperandsTable(
    sol::this_state ts, const LowLevelILInstruction& instr);

// Depth-first pre-order walk. Invokes ``cb(sub_instr)`` at each node
// and accumulates every non-nil return into a 1-indexed result table,
// mirroring python/lowlevelil.py:741 traverse semantics.
sol::table TraverseLLILInstruction(sol::this_state ts,
                                    const LowLevelILInstruction& instr,
                                    sol::function cb);

// ---- MLIL analogs (R9.2 commit B) ----

// Per-opcode dispatch for MLIL. Returns a reference to a static empty
// vector when the opcode has no detailed_operands override in Python
// (MLIL_NOP, MLIL_NORET, MLIL_BP, MLIL_UNDEF, MLIL_UNIMPL, plus any
// opcodes whose subclass is not concretely defined). Generated in
// bindings/il_operands_table.inc alongside the LLIL dispatcher.
const std::vector<MLILOperandSpec>& MLILOperandSpecsForOperation(
    BNMediumLevelILOperation op);

// Resolve the owning Architecture for a MediumLevelILInstruction, or
// nullptr when the IL function has no attached BN Function. Mirrors
// the LLIL overload above.
Ref<Architecture> ArchFor(const MediumLevelILInstruction& instr);

// Project a single MLIL operand spec to a Lua value. Tag vocabulary
// is the MLIL-specific set documented at
// docs/il-metatable-design.md section 12.3: "int", "float", "expr",
// "expr_list", "int_list", "var", "var_ssa", "var_list",
// "var_ssa_list", "var_ssa_dest_and_src", "ConstantData",
// "target_map", "intrinsic", "cond". Unknown tags (including LLIL-
// only tags) return sol::nil.
sol::object MLILOperandToLua(sol::state_view lua,
                              const MediumLevelILInstruction& instr,
                              const MLILOperandSpec& spec);

// Build the 1-indexed `instr.operands` list for an MLIL instruction.
sol::table BuildMLILOperandsTable(sol::this_state ts,
                                   const MediumLevelILInstruction& instr);

// Build the 1-indexed `instr.detailed_operands` list for an MLIL
// instruction. Each entry is {name=..., value=..., type=...}.
sol::table BuildMLILDetailedOperandsTable(
    sol::this_state ts, const MediumLevelILInstruction& instr);

// Prefix-order flattened walk of the MLIL expression tree.
sol::table BuildMLILPrefixOperandsTable(
    sol::this_state ts, const MediumLevelILInstruction& instr);

// Depth-first pre-order walk for MLIL.
sol::table TraverseMLILInstruction(sol::this_state ts,
                                    const MediumLevelILInstruction& instr,
                                    sol::function cb);

}  // namespace BinjaLua
