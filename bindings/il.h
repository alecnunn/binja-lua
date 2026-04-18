// IL binding declarations for binja-lua (LLIL / MLIL / HLIL wave).
//
// R9.1 commit 2a: LLILOperandSpec struct + per-opcode dispatch
// declaration. R9.1 commit 2b: projection helpers + traverse worker.
//
// MLIL and HLIL siblings (R9.2 / R9.3) will add MLILOperandSpec and
// HLILOperandSpec structs here when they land.

#pragma once

#include "common.h"

// Full definitions of LowLevelILInstruction and the LowLevelIL*List
// container classes. binaryninjaapi.h only forward-declares them
// (line 14052), so we need the full header to call GetRawOperandAs*
// methods and to iterate the list containers.
#include "lowlevelilinstruction.h"

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

}  // namespace BinjaLua
