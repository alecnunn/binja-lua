// Auto-generated from binaryninja-api/python/lowlevelil.py and
// binaryninja-api/python/mediumlevelil.py.
// Generator: scripts/generate_il_tables.py (tracked, run by hand).
// Do NOT hand-edit. Regenerate after bumping the BN submodule.
//
// Contents:
//   - LLILOperandSpecsForOperation(BNLowLevelILOperation) returning
//     the per-opcode const vector<LLILOperandSpec>& used by
//     LLILOperandToLua to project raw instruction operands into Lua
//     values. Originally landed in R9.1 commit 2a.
//   - MLILOperandSpecsForOperation(BNMediumLevelILOperation) returning
//     the per-opcode const vector<MLILOperandSpec>& used by
//     MLILOperandToLua. Added in R9.2 commit B.
//
// *OperandSpec shape (structs defined in bindings/il.h, field-identical
// between the two families):
//     struct LLILOperandSpec { const char* name; const char* type_tag;
//                              uint8_t slot_first; uint8_t slot_last; };
//     struct MLILOperandSpec { same; };
//
// For single-slot scalars and single-slot list/map accessors, slot_last
// == slot_first. For explicit-pair accessors (reg_ssa, flag_ssa,
// reg_stack_ssa, var_ssa, var_ssa_dest_and_src, ConstantData),
// slot_last == slot_first + 1. Encodings match the Python
// per-subclass detailed_operands overrides; see
// scripts/generate_il_tables.py ACCESSOR_TO_TAG mapping.
//
// Opcodes with empty detailed_operands (LLIL_NOP, LLIL_NORET, LLIL_POP,
// LLIL_SYSCALL, LLIL_BP, LLIL_UNDEF, LLIL_UNIMPL, MLIL_NOP,
// MLIL_NORET, MLIL_BP, MLIL_UNDEF, MLIL_UNIMPL, plus opcodes whose
// Python subclass is not concretely defined) map to an empty spec
// vector via each switch's default arm.
//
// This file is included by bindings/il_operand_conv.cpp inside the
// BinjaLua namespace.

// ---- BNLowLevelILOperation operand specs ----

static const std::vector<LLILOperandSpec> kLLILOperandSpecsEmpty{};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG{
    {"dest", "reg", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG_SPLIT{
    {"hi", "reg", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"lo", "reg", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_FLAG{
    {"dest", "flag", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG_STACK_REL{
    {"stack", "reg_stack", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_PUSH{
    {"stack", "reg_stack", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_LOAD{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_STORE{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_PUSH{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG{
    {"src", "reg", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_SPLIT{
    {"hi", "reg", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"lo", "reg", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_REL{
    {"stack", "reg_stack", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_POP{
    {"stack", "reg_stack", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_FREE_REG{
    {"dest", "reg", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_FREE_REL{
    {"stack", "reg_stack", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CONST{
    {"constant", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CONST_PTR{
    {"constant", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_EXTERN_PTR{
    {"constant", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLOAT_CONST{
    {"constant", "float", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLAG{
    {"src", "flag", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLAG_BIT{
    {"src", "flag", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"bit", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ADD{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ADC{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SUB{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SBB{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_AND{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_OR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_XOR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_LSL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_LSR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ASR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ROL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_RLC{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ROR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_RRC{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MUL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MULU_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MULS_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_DIVU{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_DIVU_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_DIVS{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_DIVS_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MODU{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MODU_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MODS{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MODS_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_NEG{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_NOT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SX{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ZX{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_LOW_PART{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_JUMP{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_JUMP_TO{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"targets", "target_map", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CALL{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CALL_STACK_ADJUST{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"stack_adjustment", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"reg_stack_adjustments", "reg_stack_adjust", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_TAILCALL{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_RET{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_IF{
    {"condition", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"true", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"false", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_GOTO{
    {"dest", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLAG_COND{
    {"condition", "cond", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"semantic_class", "sem_class", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLAG_GROUP{
    {"semantic_group", "sem_group", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_E{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_NE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_SLT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_ULT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_SLE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_ULE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_SGE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_UGE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_SGT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CMP_UGT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_TEST_BIT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_BOOL_TO_INT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ADD_OVERFLOW{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_TRAP{
    {"vector", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_INTRINSIC{
    {"output", "reg_or_flag_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"intrinsic", "intrinsic", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_UNIMPL_MEM{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FADD{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FSUB{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FMUL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FDIV{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FSQRT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FNEG{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FABS{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLOAT_TO_INT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_INT_TO_FLOAT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLOAT_CONV{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_ROUND_TO_INT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLOOR{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CEIL{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FTRUNC{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_E{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_NE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_LT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_LE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_GE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_GT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_O{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FCMP_UO{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG_SSA{
    {"dest", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG_SSA_PARTIAL{
    {"full_reg", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"dest", "reg", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG_SPLIT_SSA{
    {"hi", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"lo", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG_STACK_REL_SSA{
    {"stack", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"top", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_REG_STACK_ABS_SSA{
    {"stack", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "reg", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_SPLIT_DEST_SSA{
    {"dest", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_DEST_SSA{
    {"dest", "reg_stack_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "reg_stack_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_SSA{
    {"src", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_SSA_PARTIAL{
    {"full_reg", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "reg", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_SPLIT_SSA{
    {"hi", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"lo", "reg_ssa", static_cast<uint8_t>(2), static_cast<uint8_t>(3)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_REL_SSA{
    {"stack", "reg_stack_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"top", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_ABS_SSA{
    {"stack", "reg_stack_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "reg", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_FREE_REL_SSA{
    {"stack", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"top", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_FREE_ABS_SSA{
    {"stack", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "reg", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SET_FLAG_SSA{
    {"dest", "flag_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLAG_SSA{
    {"src", "flag_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLAG_BIT_SSA{
    {"src", "flag_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"bit", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CALL_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"stack", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "unknown", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SYSCALL_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"stack_reg", "unknown", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"stack_memory", "unknown", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"params", "unknown", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_TAILCALL_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"stack", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "unknown", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CALL_PARAM{
    {"src", "expr_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CALL_STACK_SSA{
    {"src", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src_memory", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_CALL_OUTPUT_SSA{
    {"dest_memory", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "reg_ssa_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SEPARATE_PARAM_LIST_SSA{
    {"src", "expr_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_SHARED_PARAM_SLOT_SSA{
    {"src", "expr_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MEMORY_INTRINSIC_OUTPUT_SSA{
    {"dest_memory", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output", "reg_or_flag_ssa_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_LOAD_SSA{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src_memory", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_STORE_SSA{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest_memory", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src_memory", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_INTRINSIC_SSA{
    {"output", "reg_or_flag_ssa_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"intrinsic", "intrinsic", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "unknown", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MEMORY_INTRINSIC_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"intrinsic", "intrinsic", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "unknown", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src_memory", "int", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_PHI{
    {"dest", "reg_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "reg_ssa_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_REG_STACK_PHI{
    {"dest", "reg_stack_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "reg_stack_ssa_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_FLAG_PHI{
    {"dest", "flag_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "flag_ssa_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<LLILOperandSpec> kLLILOperandSpecs_LLIL_MEM_PHI{
    {"dest_memory", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src_memory", "int_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

const std::vector<LLILOperandSpec>& LLILOperandSpecsForOperation(BNLowLevelILOperation op) {
    switch (op) {
        case LLIL_SET_REG: return kLLILOperandSpecs_LLIL_SET_REG;
        case LLIL_SET_REG_SPLIT: return kLLILOperandSpecs_LLIL_SET_REG_SPLIT;
        case LLIL_SET_FLAG: return kLLILOperandSpecs_LLIL_SET_FLAG;
        case LLIL_SET_REG_STACK_REL: return kLLILOperandSpecs_LLIL_SET_REG_STACK_REL;
        case LLIL_REG_STACK_PUSH: return kLLILOperandSpecs_LLIL_REG_STACK_PUSH;
        case LLIL_LOAD: return kLLILOperandSpecs_LLIL_LOAD;
        case LLIL_STORE: return kLLILOperandSpecs_LLIL_STORE;
        case LLIL_PUSH: return kLLILOperandSpecs_LLIL_PUSH;
        case LLIL_REG: return kLLILOperandSpecs_LLIL_REG;
        case LLIL_REG_SPLIT: return kLLILOperandSpecs_LLIL_REG_SPLIT;
        case LLIL_REG_STACK_REL: return kLLILOperandSpecs_LLIL_REG_STACK_REL;
        case LLIL_REG_STACK_POP: return kLLILOperandSpecs_LLIL_REG_STACK_POP;
        case LLIL_REG_STACK_FREE_REG: return kLLILOperandSpecs_LLIL_REG_STACK_FREE_REG;
        case LLIL_REG_STACK_FREE_REL: return kLLILOperandSpecs_LLIL_REG_STACK_FREE_REL;
        case LLIL_CONST: return kLLILOperandSpecs_LLIL_CONST;
        case LLIL_CONST_PTR: return kLLILOperandSpecs_LLIL_CONST_PTR;
        case LLIL_EXTERN_PTR: return kLLILOperandSpecs_LLIL_EXTERN_PTR;
        case LLIL_FLOAT_CONST: return kLLILOperandSpecs_LLIL_FLOAT_CONST;
        case LLIL_FLAG: return kLLILOperandSpecs_LLIL_FLAG;
        case LLIL_FLAG_BIT: return kLLILOperandSpecs_LLIL_FLAG_BIT;
        case LLIL_ADD: return kLLILOperandSpecs_LLIL_ADD;
        case LLIL_ADC: return kLLILOperandSpecs_LLIL_ADC;
        case LLIL_SUB: return kLLILOperandSpecs_LLIL_SUB;
        case LLIL_SBB: return kLLILOperandSpecs_LLIL_SBB;
        case LLIL_AND: return kLLILOperandSpecs_LLIL_AND;
        case LLIL_OR: return kLLILOperandSpecs_LLIL_OR;
        case LLIL_XOR: return kLLILOperandSpecs_LLIL_XOR;
        case LLIL_LSL: return kLLILOperandSpecs_LLIL_LSL;
        case LLIL_LSR: return kLLILOperandSpecs_LLIL_LSR;
        case LLIL_ASR: return kLLILOperandSpecs_LLIL_ASR;
        case LLIL_ROL: return kLLILOperandSpecs_LLIL_ROL;
        case LLIL_RLC: return kLLILOperandSpecs_LLIL_RLC;
        case LLIL_ROR: return kLLILOperandSpecs_LLIL_ROR;
        case LLIL_RRC: return kLLILOperandSpecs_LLIL_RRC;
        case LLIL_MUL: return kLLILOperandSpecs_LLIL_MUL;
        case LLIL_MULU_DP: return kLLILOperandSpecs_LLIL_MULU_DP;
        case LLIL_MULS_DP: return kLLILOperandSpecs_LLIL_MULS_DP;
        case LLIL_DIVU: return kLLILOperandSpecs_LLIL_DIVU;
        case LLIL_DIVU_DP: return kLLILOperandSpecs_LLIL_DIVU_DP;
        case LLIL_DIVS: return kLLILOperandSpecs_LLIL_DIVS;
        case LLIL_DIVS_DP: return kLLILOperandSpecs_LLIL_DIVS_DP;
        case LLIL_MODU: return kLLILOperandSpecs_LLIL_MODU;
        case LLIL_MODU_DP: return kLLILOperandSpecs_LLIL_MODU_DP;
        case LLIL_MODS: return kLLILOperandSpecs_LLIL_MODS;
        case LLIL_MODS_DP: return kLLILOperandSpecs_LLIL_MODS_DP;
        case LLIL_NEG: return kLLILOperandSpecs_LLIL_NEG;
        case LLIL_NOT: return kLLILOperandSpecs_LLIL_NOT;
        case LLIL_SX: return kLLILOperandSpecs_LLIL_SX;
        case LLIL_ZX: return kLLILOperandSpecs_LLIL_ZX;
        case LLIL_LOW_PART: return kLLILOperandSpecs_LLIL_LOW_PART;
        case LLIL_JUMP: return kLLILOperandSpecs_LLIL_JUMP;
        case LLIL_JUMP_TO: return kLLILOperandSpecs_LLIL_JUMP_TO;
        case LLIL_CALL: return kLLILOperandSpecs_LLIL_CALL;
        case LLIL_CALL_STACK_ADJUST: return kLLILOperandSpecs_LLIL_CALL_STACK_ADJUST;
        case LLIL_TAILCALL: return kLLILOperandSpecs_LLIL_TAILCALL;
        case LLIL_RET: return kLLILOperandSpecs_LLIL_RET;
        case LLIL_IF: return kLLILOperandSpecs_LLIL_IF;
        case LLIL_GOTO: return kLLILOperandSpecs_LLIL_GOTO;
        case LLIL_FLAG_COND: return kLLILOperandSpecs_LLIL_FLAG_COND;
        case LLIL_FLAG_GROUP: return kLLILOperandSpecs_LLIL_FLAG_GROUP;
        case LLIL_CMP_E: return kLLILOperandSpecs_LLIL_CMP_E;
        case LLIL_CMP_NE: return kLLILOperandSpecs_LLIL_CMP_NE;
        case LLIL_CMP_SLT: return kLLILOperandSpecs_LLIL_CMP_SLT;
        case LLIL_CMP_ULT: return kLLILOperandSpecs_LLIL_CMP_ULT;
        case LLIL_CMP_SLE: return kLLILOperandSpecs_LLIL_CMP_SLE;
        case LLIL_CMP_ULE: return kLLILOperandSpecs_LLIL_CMP_ULE;
        case LLIL_CMP_SGE: return kLLILOperandSpecs_LLIL_CMP_SGE;
        case LLIL_CMP_UGE: return kLLILOperandSpecs_LLIL_CMP_UGE;
        case LLIL_CMP_SGT: return kLLILOperandSpecs_LLIL_CMP_SGT;
        case LLIL_CMP_UGT: return kLLILOperandSpecs_LLIL_CMP_UGT;
        case LLIL_TEST_BIT: return kLLILOperandSpecs_LLIL_TEST_BIT;
        case LLIL_BOOL_TO_INT: return kLLILOperandSpecs_LLIL_BOOL_TO_INT;
        case LLIL_ADD_OVERFLOW: return kLLILOperandSpecs_LLIL_ADD_OVERFLOW;
        case LLIL_TRAP: return kLLILOperandSpecs_LLIL_TRAP;
        case LLIL_INTRINSIC: return kLLILOperandSpecs_LLIL_INTRINSIC;
        case LLIL_UNIMPL_MEM: return kLLILOperandSpecs_LLIL_UNIMPL_MEM;
        case LLIL_FADD: return kLLILOperandSpecs_LLIL_FADD;
        case LLIL_FSUB: return kLLILOperandSpecs_LLIL_FSUB;
        case LLIL_FMUL: return kLLILOperandSpecs_LLIL_FMUL;
        case LLIL_FDIV: return kLLILOperandSpecs_LLIL_FDIV;
        case LLIL_FSQRT: return kLLILOperandSpecs_LLIL_FSQRT;
        case LLIL_FNEG: return kLLILOperandSpecs_LLIL_FNEG;
        case LLIL_FABS: return kLLILOperandSpecs_LLIL_FABS;
        case LLIL_FLOAT_TO_INT: return kLLILOperandSpecs_LLIL_FLOAT_TO_INT;
        case LLIL_INT_TO_FLOAT: return kLLILOperandSpecs_LLIL_INT_TO_FLOAT;
        case LLIL_FLOAT_CONV: return kLLILOperandSpecs_LLIL_FLOAT_CONV;
        case LLIL_ROUND_TO_INT: return kLLILOperandSpecs_LLIL_ROUND_TO_INT;
        case LLIL_FLOOR: return kLLILOperandSpecs_LLIL_FLOOR;
        case LLIL_CEIL: return kLLILOperandSpecs_LLIL_CEIL;
        case LLIL_FTRUNC: return kLLILOperandSpecs_LLIL_FTRUNC;
        case LLIL_FCMP_E: return kLLILOperandSpecs_LLIL_FCMP_E;
        case LLIL_FCMP_NE: return kLLILOperandSpecs_LLIL_FCMP_NE;
        case LLIL_FCMP_LT: return kLLILOperandSpecs_LLIL_FCMP_LT;
        case LLIL_FCMP_LE: return kLLILOperandSpecs_LLIL_FCMP_LE;
        case LLIL_FCMP_GE: return kLLILOperandSpecs_LLIL_FCMP_GE;
        case LLIL_FCMP_GT: return kLLILOperandSpecs_LLIL_FCMP_GT;
        case LLIL_FCMP_O: return kLLILOperandSpecs_LLIL_FCMP_O;
        case LLIL_FCMP_UO: return kLLILOperandSpecs_LLIL_FCMP_UO;
        case LLIL_SET_REG_SSA: return kLLILOperandSpecs_LLIL_SET_REG_SSA;
        case LLIL_SET_REG_SSA_PARTIAL: return kLLILOperandSpecs_LLIL_SET_REG_SSA_PARTIAL;
        case LLIL_SET_REG_SPLIT_SSA: return kLLILOperandSpecs_LLIL_SET_REG_SPLIT_SSA;
        case LLIL_SET_REG_STACK_REL_SSA: return kLLILOperandSpecs_LLIL_SET_REG_STACK_REL_SSA;
        case LLIL_SET_REG_STACK_ABS_SSA: return kLLILOperandSpecs_LLIL_SET_REG_STACK_ABS_SSA;
        case LLIL_REG_SPLIT_DEST_SSA: return kLLILOperandSpecs_LLIL_REG_SPLIT_DEST_SSA;
        case LLIL_REG_STACK_DEST_SSA: return kLLILOperandSpecs_LLIL_REG_STACK_DEST_SSA;
        case LLIL_REG_SSA: return kLLILOperandSpecs_LLIL_REG_SSA;
        case LLIL_REG_SSA_PARTIAL: return kLLILOperandSpecs_LLIL_REG_SSA_PARTIAL;
        case LLIL_REG_SPLIT_SSA: return kLLILOperandSpecs_LLIL_REG_SPLIT_SSA;
        case LLIL_REG_STACK_REL_SSA: return kLLILOperandSpecs_LLIL_REG_STACK_REL_SSA;
        case LLIL_REG_STACK_ABS_SSA: return kLLILOperandSpecs_LLIL_REG_STACK_ABS_SSA;
        case LLIL_REG_STACK_FREE_REL_SSA: return kLLILOperandSpecs_LLIL_REG_STACK_FREE_REL_SSA;
        case LLIL_REG_STACK_FREE_ABS_SSA: return kLLILOperandSpecs_LLIL_REG_STACK_FREE_ABS_SSA;
        case LLIL_SET_FLAG_SSA: return kLLILOperandSpecs_LLIL_SET_FLAG_SSA;
        case LLIL_FLAG_SSA: return kLLILOperandSpecs_LLIL_FLAG_SSA;
        case LLIL_FLAG_BIT_SSA: return kLLILOperandSpecs_LLIL_FLAG_BIT_SSA;
        case LLIL_CALL_SSA: return kLLILOperandSpecs_LLIL_CALL_SSA;
        case LLIL_SYSCALL_SSA: return kLLILOperandSpecs_LLIL_SYSCALL_SSA;
        case LLIL_TAILCALL_SSA: return kLLILOperandSpecs_LLIL_TAILCALL_SSA;
        case LLIL_CALL_PARAM: return kLLILOperandSpecs_LLIL_CALL_PARAM;
        case LLIL_CALL_STACK_SSA: return kLLILOperandSpecs_LLIL_CALL_STACK_SSA;
        case LLIL_CALL_OUTPUT_SSA: return kLLILOperandSpecs_LLIL_CALL_OUTPUT_SSA;
        case LLIL_SEPARATE_PARAM_LIST_SSA: return kLLILOperandSpecs_LLIL_SEPARATE_PARAM_LIST_SSA;
        case LLIL_SHARED_PARAM_SLOT_SSA: return kLLILOperandSpecs_LLIL_SHARED_PARAM_SLOT_SSA;
        case LLIL_MEMORY_INTRINSIC_OUTPUT_SSA: return kLLILOperandSpecs_LLIL_MEMORY_INTRINSIC_OUTPUT_SSA;
        case LLIL_LOAD_SSA: return kLLILOperandSpecs_LLIL_LOAD_SSA;
        case LLIL_STORE_SSA: return kLLILOperandSpecs_LLIL_STORE_SSA;
        case LLIL_INTRINSIC_SSA: return kLLILOperandSpecs_LLIL_INTRINSIC_SSA;
        case LLIL_MEMORY_INTRINSIC_SSA: return kLLILOperandSpecs_LLIL_MEMORY_INTRINSIC_SSA;
        case LLIL_REG_PHI: return kLLILOperandSpecs_LLIL_REG_PHI;
        case LLIL_REG_STACK_PHI: return kLLILOperandSpecs_LLIL_REG_STACK_PHI;
        case LLIL_FLAG_PHI: return kLLILOperandSpecs_LLIL_FLAG_PHI;
        case LLIL_MEM_PHI: return kLLILOperandSpecs_LLIL_MEM_PHI;
        default: return kLLILOperandSpecsEmpty;
    }
}

// ---- BNMediumLevelILOperation operand specs ----

static const std::vector<MLILOperandSpec> kMLILOperandSpecsEmpty{};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR{
    {"dest", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR_FIELD{
    {"dest", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR_SPLIT{
    {"high", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"low", "var", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_LOAD{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_LOAD_STRUCT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_STORE{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_STORE_STRUCT{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR{
    {"var", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_FIELD{
    {"src", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_SPLIT{
    {"high", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"low", "var", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ADDRESS_OF{
    {"src", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ADDRESS_OF_FIELD{
    {"src", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CONST{
    {"constant", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CONST_DATA{
    {"constant", "ConstantData", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CONST_PTR{
    {"constant", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_EXTERN_PTR{
    {"constant", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FLOAT_CONST{
    {"constant", "float", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_IMPORT{
    {"constant", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ADD{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ADC{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SUB{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SBB{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_AND{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_OR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_XOR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_LSL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_LSR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ASR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ROL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_RLC{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ROR{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_RRC{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"carry", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MUL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MULU_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MULS_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_DIVU{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_DIVU_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_DIVS{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_DIVS_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MODU{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MODU_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MODS{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MODS_DP{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_NEG{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_NOT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SX{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ZX{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_LOW_PART{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_JUMP{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_JUMP_TO{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"targets", "target_map", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_RET_HINT{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL{
    {"output", "var_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "expr_list", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL_UNTYPED{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"stack", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL_OUTPUT{
    {"dest", "var_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL_PARAM{
    {"src", "expr_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SEPARATE_PARAM_LIST{
    {"params", "expr_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SHARED_PARAM_SLOT{
    {"params", "expr_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_RET{
    {"src", "expr_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_IF{
    {"condition", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"true", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"false", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_GOTO{
    {"dest", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_E{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_NE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_SLT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_ULT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_SLE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_ULE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_SGE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_UGE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_SGT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CMP_UGT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_TEST_BIT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_BOOL_TO_INT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ADD_OVERFLOW{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SYSCALL{
    {"output", "var_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"params", "expr_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SYSCALL_UNTYPED{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"params", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"stack", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_TAILCALL{
    {"output", "var_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "expr_list", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_TAILCALL_UNTYPED{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"stack", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_INTRINSIC{
    {"output", "var_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"intrinsic", "intrinsic", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "expr_list", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FREE_VAR_SLOT{
    {"dest", "var", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_TRAP{
    {"vector", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_UNIMPL_MEM{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FADD{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FSUB{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FMUL{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FDIV{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FSQRT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FNEG{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FABS{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FLOAT_TO_INT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_INT_TO_FLOAT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FLOAT_CONV{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_ROUND_TO_INT{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FLOOR{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CEIL{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FTRUNC{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_E{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_NE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_LT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_LE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_GE{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_GT{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_O{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FCMP_UO{
    {"left", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"right", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR_SSA{
    {"dest", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR_SSA_FIELD{
    {"dest", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"prev", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(2)},
    {"offset", "int", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
    {"src", "expr", static_cast<uint8_t>(4), static_cast<uint8_t>(4)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR_SPLIT_SSA{
    {"high", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"low", "var_ssa", static_cast<uint8_t>(2), static_cast<uint8_t>(3)},
    {"src", "expr", static_cast<uint8_t>(4), static_cast<uint8_t>(4)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR_ALIASED{
    {"dest", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"prev", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(2)},
    {"src", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SET_VAR_ALIASED_FIELD{
    {"dest", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"prev", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(2)},
    {"offset", "int", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
    {"src", "expr", static_cast<uint8_t>(4), static_cast<uint8_t>(4)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_SSA{
    {"var", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_SSA_FIELD{
    {"src", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"offset", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_ALIASED{
    {"src", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_ALIASED_FIELD{
    {"src", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"offset", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_SPLIT_SSA{
    {"high", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"low", "var_ssa", static_cast<uint8_t>(2), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output_dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "expr_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src_memory", "int", static_cast<uint8_t>(4), static_cast<uint8_t>(4)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL_UNTYPED_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output_dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params_src_memory", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"stack", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SYSCALL_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output_dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"params", "expr_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src_memory", "int", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_SYSCALL_UNTYPED_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output_dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"params", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params_src_memory", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"stack", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_TAILCALL_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output_dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "expr_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src_memory", "int", static_cast<uint8_t>(4), static_cast<uint8_t>(4)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_TAILCALL_UNTYPED_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output_dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "expr", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "expr", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"stack", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL_PARAM_SSA{
    {"src_memory", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src", "expr_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_CALL_OUTPUT_SSA{
    {"dest_memory", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest", "var_ssa_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MEMORY_INTRINSIC_OUTPUT_SSA{
    {"dest_memory", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"output", "var_ssa_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_LOAD_SSA{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src_memory", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_LOAD_STRUCT_SSA{
    {"src", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src_memory", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_STORE_SSA{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest_memory", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"src_memory", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src", "expr", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_STORE_STRUCT_SSA{
    {"dest", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"offset", "int", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"dest_memory", "int", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src_memory", "int", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
    {"src", "expr", static_cast<uint8_t>(4), static_cast<uint8_t>(4)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_INTRINSIC_SSA{
    {"output", "var_ssa_list", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"intrinsic", "intrinsic", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"params", "expr_list", static_cast<uint8_t>(3), static_cast<uint8_t>(3)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MEMORY_INTRINSIC_SSA{
    {"output", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"dest_memory", "expr", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"intrinsic", "intrinsic", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
    {"params", "expr_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
    {"src_memory", "int", static_cast<uint8_t>(4), static_cast<uint8_t>(4)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_FREE_VAR_SLOT_SSA{
    {"dest", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"prev", "var_ssa_dest_and_src", static_cast<uint8_t>(0), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_VAR_PHI{
    {"dest", "var_ssa", static_cast<uint8_t>(0), static_cast<uint8_t>(1)},
    {"src", "var_ssa_list", static_cast<uint8_t>(2), static_cast<uint8_t>(2)},
};

static const std::vector<MLILOperandSpec> kMLILOperandSpecs_MLIL_MEM_PHI{
    {"dest_memory", "int", static_cast<uint8_t>(0), static_cast<uint8_t>(0)},
    {"src_memory", "int_list", static_cast<uint8_t>(1), static_cast<uint8_t>(1)},
};

const std::vector<MLILOperandSpec>& MLILOperandSpecsForOperation(BNMediumLevelILOperation op) {
    switch (op) {
        case MLIL_SET_VAR: return kMLILOperandSpecs_MLIL_SET_VAR;
        case MLIL_SET_VAR_FIELD: return kMLILOperandSpecs_MLIL_SET_VAR_FIELD;
        case MLIL_SET_VAR_SPLIT: return kMLILOperandSpecs_MLIL_SET_VAR_SPLIT;
        case MLIL_LOAD: return kMLILOperandSpecs_MLIL_LOAD;
        case MLIL_LOAD_STRUCT: return kMLILOperandSpecs_MLIL_LOAD_STRUCT;
        case MLIL_STORE: return kMLILOperandSpecs_MLIL_STORE;
        case MLIL_STORE_STRUCT: return kMLILOperandSpecs_MLIL_STORE_STRUCT;
        case MLIL_VAR: return kMLILOperandSpecs_MLIL_VAR;
        case MLIL_VAR_FIELD: return kMLILOperandSpecs_MLIL_VAR_FIELD;
        case MLIL_VAR_SPLIT: return kMLILOperandSpecs_MLIL_VAR_SPLIT;
        case MLIL_ADDRESS_OF: return kMLILOperandSpecs_MLIL_ADDRESS_OF;
        case MLIL_ADDRESS_OF_FIELD: return kMLILOperandSpecs_MLIL_ADDRESS_OF_FIELD;
        case MLIL_CONST: return kMLILOperandSpecs_MLIL_CONST;
        case MLIL_CONST_DATA: return kMLILOperandSpecs_MLIL_CONST_DATA;
        case MLIL_CONST_PTR: return kMLILOperandSpecs_MLIL_CONST_PTR;
        case MLIL_EXTERN_PTR: return kMLILOperandSpecs_MLIL_EXTERN_PTR;
        case MLIL_FLOAT_CONST: return kMLILOperandSpecs_MLIL_FLOAT_CONST;
        case MLIL_IMPORT: return kMLILOperandSpecs_MLIL_IMPORT;
        case MLIL_ADD: return kMLILOperandSpecs_MLIL_ADD;
        case MLIL_ADC: return kMLILOperandSpecs_MLIL_ADC;
        case MLIL_SUB: return kMLILOperandSpecs_MLIL_SUB;
        case MLIL_SBB: return kMLILOperandSpecs_MLIL_SBB;
        case MLIL_AND: return kMLILOperandSpecs_MLIL_AND;
        case MLIL_OR: return kMLILOperandSpecs_MLIL_OR;
        case MLIL_XOR: return kMLILOperandSpecs_MLIL_XOR;
        case MLIL_LSL: return kMLILOperandSpecs_MLIL_LSL;
        case MLIL_LSR: return kMLILOperandSpecs_MLIL_LSR;
        case MLIL_ASR: return kMLILOperandSpecs_MLIL_ASR;
        case MLIL_ROL: return kMLILOperandSpecs_MLIL_ROL;
        case MLIL_RLC: return kMLILOperandSpecs_MLIL_RLC;
        case MLIL_ROR: return kMLILOperandSpecs_MLIL_ROR;
        case MLIL_RRC: return kMLILOperandSpecs_MLIL_RRC;
        case MLIL_MUL: return kMLILOperandSpecs_MLIL_MUL;
        case MLIL_MULU_DP: return kMLILOperandSpecs_MLIL_MULU_DP;
        case MLIL_MULS_DP: return kMLILOperandSpecs_MLIL_MULS_DP;
        case MLIL_DIVU: return kMLILOperandSpecs_MLIL_DIVU;
        case MLIL_DIVU_DP: return kMLILOperandSpecs_MLIL_DIVU_DP;
        case MLIL_DIVS: return kMLILOperandSpecs_MLIL_DIVS;
        case MLIL_DIVS_DP: return kMLILOperandSpecs_MLIL_DIVS_DP;
        case MLIL_MODU: return kMLILOperandSpecs_MLIL_MODU;
        case MLIL_MODU_DP: return kMLILOperandSpecs_MLIL_MODU_DP;
        case MLIL_MODS: return kMLILOperandSpecs_MLIL_MODS;
        case MLIL_MODS_DP: return kMLILOperandSpecs_MLIL_MODS_DP;
        case MLIL_NEG: return kMLILOperandSpecs_MLIL_NEG;
        case MLIL_NOT: return kMLILOperandSpecs_MLIL_NOT;
        case MLIL_SX: return kMLILOperandSpecs_MLIL_SX;
        case MLIL_ZX: return kMLILOperandSpecs_MLIL_ZX;
        case MLIL_LOW_PART: return kMLILOperandSpecs_MLIL_LOW_PART;
        case MLIL_JUMP: return kMLILOperandSpecs_MLIL_JUMP;
        case MLIL_JUMP_TO: return kMLILOperandSpecs_MLIL_JUMP_TO;
        case MLIL_RET_HINT: return kMLILOperandSpecs_MLIL_RET_HINT;
        case MLIL_CALL: return kMLILOperandSpecs_MLIL_CALL;
        case MLIL_CALL_UNTYPED: return kMLILOperandSpecs_MLIL_CALL_UNTYPED;
        case MLIL_CALL_OUTPUT: return kMLILOperandSpecs_MLIL_CALL_OUTPUT;
        case MLIL_CALL_PARAM: return kMLILOperandSpecs_MLIL_CALL_PARAM;
        case MLIL_SEPARATE_PARAM_LIST: return kMLILOperandSpecs_MLIL_SEPARATE_PARAM_LIST;
        case MLIL_SHARED_PARAM_SLOT: return kMLILOperandSpecs_MLIL_SHARED_PARAM_SLOT;
        case MLIL_RET: return kMLILOperandSpecs_MLIL_RET;
        case MLIL_IF: return kMLILOperandSpecs_MLIL_IF;
        case MLIL_GOTO: return kMLILOperandSpecs_MLIL_GOTO;
        case MLIL_CMP_E: return kMLILOperandSpecs_MLIL_CMP_E;
        case MLIL_CMP_NE: return kMLILOperandSpecs_MLIL_CMP_NE;
        case MLIL_CMP_SLT: return kMLILOperandSpecs_MLIL_CMP_SLT;
        case MLIL_CMP_ULT: return kMLILOperandSpecs_MLIL_CMP_ULT;
        case MLIL_CMP_SLE: return kMLILOperandSpecs_MLIL_CMP_SLE;
        case MLIL_CMP_ULE: return kMLILOperandSpecs_MLIL_CMP_ULE;
        case MLIL_CMP_SGE: return kMLILOperandSpecs_MLIL_CMP_SGE;
        case MLIL_CMP_UGE: return kMLILOperandSpecs_MLIL_CMP_UGE;
        case MLIL_CMP_SGT: return kMLILOperandSpecs_MLIL_CMP_SGT;
        case MLIL_CMP_UGT: return kMLILOperandSpecs_MLIL_CMP_UGT;
        case MLIL_TEST_BIT: return kMLILOperandSpecs_MLIL_TEST_BIT;
        case MLIL_BOOL_TO_INT: return kMLILOperandSpecs_MLIL_BOOL_TO_INT;
        case MLIL_ADD_OVERFLOW: return kMLILOperandSpecs_MLIL_ADD_OVERFLOW;
        case MLIL_SYSCALL: return kMLILOperandSpecs_MLIL_SYSCALL;
        case MLIL_SYSCALL_UNTYPED: return kMLILOperandSpecs_MLIL_SYSCALL_UNTYPED;
        case MLIL_TAILCALL: return kMLILOperandSpecs_MLIL_TAILCALL;
        case MLIL_TAILCALL_UNTYPED: return kMLILOperandSpecs_MLIL_TAILCALL_UNTYPED;
        case MLIL_INTRINSIC: return kMLILOperandSpecs_MLIL_INTRINSIC;
        case MLIL_FREE_VAR_SLOT: return kMLILOperandSpecs_MLIL_FREE_VAR_SLOT;
        case MLIL_TRAP: return kMLILOperandSpecs_MLIL_TRAP;
        case MLIL_UNIMPL_MEM: return kMLILOperandSpecs_MLIL_UNIMPL_MEM;
        case MLIL_FADD: return kMLILOperandSpecs_MLIL_FADD;
        case MLIL_FSUB: return kMLILOperandSpecs_MLIL_FSUB;
        case MLIL_FMUL: return kMLILOperandSpecs_MLIL_FMUL;
        case MLIL_FDIV: return kMLILOperandSpecs_MLIL_FDIV;
        case MLIL_FSQRT: return kMLILOperandSpecs_MLIL_FSQRT;
        case MLIL_FNEG: return kMLILOperandSpecs_MLIL_FNEG;
        case MLIL_FABS: return kMLILOperandSpecs_MLIL_FABS;
        case MLIL_FLOAT_TO_INT: return kMLILOperandSpecs_MLIL_FLOAT_TO_INT;
        case MLIL_INT_TO_FLOAT: return kMLILOperandSpecs_MLIL_INT_TO_FLOAT;
        case MLIL_FLOAT_CONV: return kMLILOperandSpecs_MLIL_FLOAT_CONV;
        case MLIL_ROUND_TO_INT: return kMLILOperandSpecs_MLIL_ROUND_TO_INT;
        case MLIL_FLOOR: return kMLILOperandSpecs_MLIL_FLOOR;
        case MLIL_CEIL: return kMLILOperandSpecs_MLIL_CEIL;
        case MLIL_FTRUNC: return kMLILOperandSpecs_MLIL_FTRUNC;
        case MLIL_FCMP_E: return kMLILOperandSpecs_MLIL_FCMP_E;
        case MLIL_FCMP_NE: return kMLILOperandSpecs_MLIL_FCMP_NE;
        case MLIL_FCMP_LT: return kMLILOperandSpecs_MLIL_FCMP_LT;
        case MLIL_FCMP_LE: return kMLILOperandSpecs_MLIL_FCMP_LE;
        case MLIL_FCMP_GE: return kMLILOperandSpecs_MLIL_FCMP_GE;
        case MLIL_FCMP_GT: return kMLILOperandSpecs_MLIL_FCMP_GT;
        case MLIL_FCMP_O: return kMLILOperandSpecs_MLIL_FCMP_O;
        case MLIL_FCMP_UO: return kMLILOperandSpecs_MLIL_FCMP_UO;
        case MLIL_SET_VAR_SSA: return kMLILOperandSpecs_MLIL_SET_VAR_SSA;
        case MLIL_SET_VAR_SSA_FIELD: return kMLILOperandSpecs_MLIL_SET_VAR_SSA_FIELD;
        case MLIL_SET_VAR_SPLIT_SSA: return kMLILOperandSpecs_MLIL_SET_VAR_SPLIT_SSA;
        case MLIL_SET_VAR_ALIASED: return kMLILOperandSpecs_MLIL_SET_VAR_ALIASED;
        case MLIL_SET_VAR_ALIASED_FIELD: return kMLILOperandSpecs_MLIL_SET_VAR_ALIASED_FIELD;
        case MLIL_VAR_SSA: return kMLILOperandSpecs_MLIL_VAR_SSA;
        case MLIL_VAR_SSA_FIELD: return kMLILOperandSpecs_MLIL_VAR_SSA_FIELD;
        case MLIL_VAR_ALIASED: return kMLILOperandSpecs_MLIL_VAR_ALIASED;
        case MLIL_VAR_ALIASED_FIELD: return kMLILOperandSpecs_MLIL_VAR_ALIASED_FIELD;
        case MLIL_VAR_SPLIT_SSA: return kMLILOperandSpecs_MLIL_VAR_SPLIT_SSA;
        case MLIL_CALL_SSA: return kMLILOperandSpecs_MLIL_CALL_SSA;
        case MLIL_CALL_UNTYPED_SSA: return kMLILOperandSpecs_MLIL_CALL_UNTYPED_SSA;
        case MLIL_SYSCALL_SSA: return kMLILOperandSpecs_MLIL_SYSCALL_SSA;
        case MLIL_SYSCALL_UNTYPED_SSA: return kMLILOperandSpecs_MLIL_SYSCALL_UNTYPED_SSA;
        case MLIL_TAILCALL_SSA: return kMLILOperandSpecs_MLIL_TAILCALL_SSA;
        case MLIL_TAILCALL_UNTYPED_SSA: return kMLILOperandSpecs_MLIL_TAILCALL_UNTYPED_SSA;
        case MLIL_CALL_PARAM_SSA: return kMLILOperandSpecs_MLIL_CALL_PARAM_SSA;
        case MLIL_CALL_OUTPUT_SSA: return kMLILOperandSpecs_MLIL_CALL_OUTPUT_SSA;
        case MLIL_MEMORY_INTRINSIC_OUTPUT_SSA: return kMLILOperandSpecs_MLIL_MEMORY_INTRINSIC_OUTPUT_SSA;
        case MLIL_LOAD_SSA: return kMLILOperandSpecs_MLIL_LOAD_SSA;
        case MLIL_LOAD_STRUCT_SSA: return kMLILOperandSpecs_MLIL_LOAD_STRUCT_SSA;
        case MLIL_STORE_SSA: return kMLILOperandSpecs_MLIL_STORE_SSA;
        case MLIL_STORE_STRUCT_SSA: return kMLILOperandSpecs_MLIL_STORE_STRUCT_SSA;
        case MLIL_INTRINSIC_SSA: return kMLILOperandSpecs_MLIL_INTRINSIC_SSA;
        case MLIL_MEMORY_INTRINSIC_SSA: return kMLILOperandSpecs_MLIL_MEMORY_INTRINSIC_SSA;
        case MLIL_FREE_VAR_SLOT_SSA: return kMLILOperandSpecs_MLIL_FREE_VAR_SLOT_SSA;
        case MLIL_VAR_PHI: return kMLILOperandSpecs_MLIL_VAR_PHI;
        case MLIL_MEM_PHI: return kMLILOperandSpecs_MLIL_MEM_PHI;
        default: return kMLILOperandSpecsEmpty;
    }
}
