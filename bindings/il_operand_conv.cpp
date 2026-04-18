// IL operand projection for binja-lua (LLIL scope, R9.1).
//
// Value-usertype note (first-in-plugin): once the R9.1 instruction
// usertype lands (commit 3 of the R9.1 wave), LLILInstruction will be
// the first non-Ref<T> value-semantics usertype registered in this
// plugin. Every previous usertype wraps a BN Ref<T> through sol2's
// unique_usertype_traits specialization in sol_config.h. BN's
// LowLevelILInstruction is a value type that internally carries a
// Ref<LowLevelILFunction>; sol2 stores it by value. Copies are cheap
// (pointer + two size_t) and safe. Do NOT try to route
// LLILInstruction through the Ref<T> traits. Operands are projected
// to plain Lua values (primitives, existing BN usertypes, nested
// instruction usertypes once registered, or plain Lua tables). There
// are no *Operand usertypes; see docs/il-metatable-design.md for the
// full rationale.
//
// sol2 3.3.0 + MSVC HARD RULE: NEVER combine sol::property with
// sol::this_state in a lambda capture or parameter list. Every
// accessor that needs the Lua state must be registered as a plain
// method (colon call on the Lua side) with sol::this_state as its
// first PARAMETER. Same rule enforced in bindings/architecture.cpp
// line 154, bindings/binaryview.cpp line 45, bindings/function.cpp
// line 137, bindings/flowgraph.cpp line 102, bindings/settings.cpp.
// See docs/session-state.md section 5 and docs/extension-plan.md
// section 4.0 for the history.
//
// File layout (R9.1 commit 2a adds the operand-spec table):
//   - bindings/il_enums.inc is a GENERATED fragment holding
//     EnumToString / EnumFromString for BNLowLevelILOperation (143)
//     and BNLowLevelILFlagCondition (22). Produced by
//     scripts/generate_il_tables.py from binaryninjacore.h.
//   - bindings/il_operands_table.inc is a GENERATED fragment holding
//     the per-opcode LLILOperandSpec vectors plus the
//     LLILOperandSpecsForOperation switch. Produced by the same
//     generator from python/lowlevelil.py's per-subclass
//     detailed_operands overrides.
//   - bindings/il.h declares the LLILOperandSpec struct used by the
//     operand table. Hand-written.
//   - bindings/il_operand_conv.cpp (THIS file) is HAND-WRITTEN glue.
//     Today it #includes the two generated fragments inside the
//     BinjaLua namespace so their symbols link correctly. Later R9.1
//     commits (2b onwards) will add LLILOperandToLua (the type_tag
//     dispatcher), BuildOperandsTable / BuildDetailedOperandsTable /
//     BuildPrefixOperandsTable, an ArchFor helper, and the traverse
//     worker.
//
// Do NOT hand-edit either .inc fragment. If the vocabulary needs to
// change, patch scripts/generate_il_tables.py and re-run it. If an
// enumerator is added (new BN opcode), bump the BN submodule and
// re-run the generator.

#include "common.h"
#include "il.h"

namespace BinjaLua {

#include "il_enums.inc"

#include "il_operands_table.inc"

}  // namespace BinjaLua
