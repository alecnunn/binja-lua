// IL binding declarations for binja-lua (LLIL / MLIL / HLIL wave).
//
// R9.1 commit 2a scope: LLILOperandSpec struct + forward declaration
// of the per-opcode dispatch function. The helper definitions
// (LLILOperandToLua / Build*OperandsTable / traverse worker) land in
// a later commit; this header only exposes what il_operands_table.inc
// needs to compile inside bindings/il_operand_conv.cpp.
//
// MLIL and HLIL siblings (R9.2 / R9.3) will add MLILOperandSpec and
// HLILOperandSpec structs here when they land.

#pragma once

#include "common.h"

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

// Per-opcode dispatch. Returns a reference to a static empty vector
// when the opcode has no detailed_operands override in Python (e.g.
// LLIL_NOP, LLIL_POP, LLIL_NORET, LLIL_SYSCALL, LLIL_BP, LLIL_UNDEF,
// LLIL_UNIMPL). Generated implementation lives in
// bindings/il_operands_table.inc.
const std::vector<LLILOperandSpec>& LLILOperandSpecsForOperation(
    BNLowLevelILOperation op);

}  // namespace BinjaLua
