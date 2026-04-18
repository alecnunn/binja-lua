// IL operand projection for binja-lua (LLIL scope, R9.1 commit 1).
//
// Value-usertype note (first-in-plugin): once the R9.1 instruction
// usertype lands (commit 4 of the R9.1 wave), LLILInstruction will be
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
// File layout (R9.1 commit 1 scope is ENUMS-ONLY):
//   - bindings/il_enums.inc is the GENERATED fragment holding
//     EnumToString / EnumFromString for BNLowLevelILOperation (143)
//     and BNLowLevelILFlagCondition (22). Produced by
//     scripts/generate_il_tables.py (local-only, NOT committed).
//     Regenerate after bumping the binaryninja-api submodule.
//   - bindings/il_operand_conv.cpp (THIS file) is HAND-WRITTEN glue.
//     Today it only #includes the generated fragment so the enum
//     helpers link into the BinjaLua namespace. Later R9.1 commits
//     will add LLILOperandToLua (operand projection dispatcher),
//     BuildOperandsTable / BuildDetailedOperandsTable /
//     BuildPrefixOperandsTable, and the traverse worker to this TU,
//     plus an il_operands_table.inc fragment (commit 2) that
//     dispatches (operation, operand_slot) to the per-operand spec.
//
// Do NOT hand-edit bindings/il_enums.inc. If the vocabulary needs to
// change, patch scripts/generate_il_tables.py and re-run it; if an
// enumerator needs to be added (new BN opcode), bump the BN submodule
// and re-run the generator.

#include "common.h"

namespace BinjaLua {

#include "il_enums.inc"

}  // namespace BinjaLua
