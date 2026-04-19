// IL operand projection for binja-lua (LLIL + MLIL scope).
//
// Value-usertype note: LLILInstruction and MLILInstruction are the
// two non-Ref<T> value-semantics usertypes registered by this
// plugin. Every other usertype wraps a BN Ref<T> through sol2's
// unique_usertype_traits specialization in sol_config.h. BN's
// LowLevelILInstruction and MediumLevelILInstruction are value
// types that internally carry a Ref<LowLevelILFunction> /
// Ref<MediumLevelILFunction>; sol2 stores them by value. Copies are
// cheap (pointer + a couple of size_t) and safe. Do NOT try to
// route either instruction type through the Ref<T> traits.
// Operands are projected to plain Lua values (primitives, existing
// BN usertypes, nested instruction usertypes, or plain Lua tables).
// There are no *Operand usertypes; see docs/il-metatable-design.md
// for the full rationale.
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
// File layout:
//   - bindings/il_enums.inc is a GENERATED fragment holding
//     EnumToString / EnumFromString for BNLowLevelILOperation (143),
//     BNLowLevelILFlagCondition (22), and BNMediumLevelILOperation
//     (140). Produced by scripts/generate_il_tables.py from
//     binaryninjacore.h.
//   - bindings/il_operands_table.inc is a GENERATED fragment holding
//     the per-opcode LLILOperandSpec vectors + LLILOperandSpecsFor-
//     Operation switch, and the MLIL analogs. Produced by the same
//     generator from python/lowlevelil.py and python/mediumlevelil.py
//     per-subclass detailed_operands overrides.
//   - bindings/il.h declares the *OperandSpec structs, the
//     projection-helper signatures, and the usertype-registration
//     entrypoints. Hand-written.
//   - bindings/il_operand_conv.cpp (THIS file) is HAND-WRITTEN glue.
//     It #includes the two generated fragments inside the BinjaLua
//     namespace, then provides the LLILOperandToLua / MLILOperandToLua
//     dispatchers, the per-instruction operands / detailed_operands
//     / prefix_operands / traverse worker family (both families), and
//     the ArchFor helpers.
//
// Do NOT hand-edit either .inc fragment. If the vocabulary needs to
// change, patch scripts/generate_il_tables.py and re-run it. If an
// enumerator is added (new BN opcode), bump the BN submodule and
// re-run the generator.

#include "common.h"
#include "il.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace BinjaLua {

#include "il_enums.inc"

#include "il_operands_table.inc"

namespace {

// Sentinel id signalling "no register". Mirrors the rule already
// applied in bindings/architecture.cpp / callingconvention.cpp where
// 0xffffffff maps to nil on the Lua side.
constexpr uint32_t kNoRegisterSentinel = 0xffffffffu;

// Build a {reg|flag = name, version = v} table for SSA entries.
sol::table MakeSSAEntry(sol::state_view lua, const char* name_key,
                         const std::string& name, size_t version) {
    sol::table t = lua.create_table(0, 2);
    if (name.empty()) {
        t[name_key] = sol::lua_nil_t{};
    } else {
        t[name_key] = name;
    }
    t["version"] = static_cast<lua_Integer>(version);
    return t;
}

// Project an expr_list slot to a 1-indexed Lua sequence of nested
// LowLevelILInstruction usertypes. sol::make_object on the element
// resolves the usertype metatable at runtime; the usertype is
// registered in commit 2c via RegisterLLILInstructionBindings.
sol::table ProjectExprList(sol::state_view lua,
                             const LowLevelILInstruction& instr,
                             size_t slot) {
    sol::table t = lua.create_table();
    int i = 1;
    for (auto sub : instr.GetRawOperandAsExprList(slot)) {
        t[i++] = sub;
    }
    return t;
}

// Option A dispatch for the 7 "unknown" delegation fields in the SSA
// call family. The generator emits `tag="unknown"` with spec.name
// carrying the original Python field name. We dispatch on (name,
// operation, spec.slot_first) without an op switch; the generator is
// the source of truth for slot_first per-site.
//
// Python shapes (python/lowlevelil.py):
//   LLIL_CALL_SSA / LLIL_TAILCALL_SSA:
//     detailed_operands = [output, dest, stack, params]
//     params <- self.param.src where param = self._get_expr(3).
//   LLIL_SYSCALL_SSA:
//     detailed_operands = [output, stack_reg, stack_memory, params]
//     stack_reg   -> direct register at slot 1
//     stack_memory -> direct integer at slot 2
//     params      -> self.param.src, param = self._get_expr(3).
//   LLIL_INTRINSIC_SSA / LLIL_MEMORY_INTRINSIC_SSA:
//     params <- self.param.src, param = self._get_expr(4).
sol::object ProjectUnknownField(sol::state_view lua,
                                 const LowLevelILInstruction& instr,
                                 const LLILOperandSpec& spec) {
    const std::string name = spec.name ? spec.name : "";
    BNLowLevelILOperation op = instr.operation;

    if (name == "params") {
        size_t param_slot = 3;
        if (op == LLIL_INTRINSIC_SSA ||
            op == LLIL_MEMORY_INTRINSIC_SSA) {
            param_slot = 4;
        }
        LowLevelILInstruction nested =
            instr.GetRawOperandAsExpr(param_slot);
        return sol::make_object(lua, ProjectExprList(lua, nested, 0));
    }
    if (op == LLIL_SYSCALL_SSA) {
        if (name == "stack_reg") {
            uint32_t idx = instr.GetRawOperandAsRegister(1);
            Ref<Architecture> arch = ArchFor(instr);
            if (!arch || idx == kNoRegisterSentinel) {
                return sol::make_object(lua, sol::lua_nil_t{});
            }
            return sol::make_object(lua, arch->GetRegisterName(idx));
        }
        if (name == "stack_memory") {
            return sol::make_object(
                lua, static_cast<lua_Integer>(
                        instr.GetRawOperandAsInteger(2)));
        }
    }
    return sol::make_object(lua, sol::lua_nil_t{});
}

}  // namespace

Ref<Architecture> ArchFor(const LowLevelILInstruction& instr) {
    if (!instr.function) return Ref<Architecture>();
    Ref<Function> f = instr.function->GetFunction();
    if (!f) return Ref<Architecture>();
    return f->GetArchitecture();
}

sol::object LLILOperandToLua(sol::state_view lua,
                              const LowLevelILInstruction& instr,
                              const LLILOperandSpec& spec) {
    const std::string tag = spec.type_tag ? spec.type_tag : "";
    const size_t slot = spec.slot_first;

    if (tag == "int") {
        uint64_t raw = instr.GetRawOperandAsInteger(slot);
        return sol::make_object(lua,
            static_cast<lua_Integer>(static_cast<int64_t>(raw)));
    }
    if (tag == "float") {
        uint64_t raw = instr.GetRawOperandAsInteger(slot);
        double d = 0.0;
        if (instr.size == 4) {
            float f = 0.0f;
            uint32_t bits = static_cast<uint32_t>(raw & 0xffffffffu);
            std::memcpy(&f, &bits, sizeof(f));
            d = static_cast<double>(f);
        } else {
            std::memcpy(&d, &raw, sizeof(d));
        }
        return sol::make_object(lua, d);
    }
    if (tag == "expr") {
        return sol::make_object(lua, instr.GetRawOperandAsExpr(slot));
    }
    if (tag == "expr_list") {
        return sol::make_object(lua, ProjectExprList(lua, instr, slot));
    }
    if (tag == "int_list") {
        sol::table t = lua.create_table();
        int i = 1;
        for (auto v : instr.GetRawOperandAsIndexList(slot)) {
            t[i++] = static_cast<lua_Integer>(v);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "reg") {
        Ref<Architecture> arch = ArchFor(instr);
        uint32_t idx = instr.GetRawOperandAsRegister(slot);
        if (!arch || idx == kNoRegisterSentinel) {
            return sol::make_object(lua, sol::lua_nil_t{});
        }
        return sol::make_object(lua, arch->GetRegisterName(idx));
    }
    if (tag == "flag") {
        Ref<Architecture> arch = ArchFor(instr);
        uint32_t idx = instr.GetRawOperandAsRegister(slot);
        if (!arch || idx == kNoRegisterSentinel) {
            return sol::make_object(lua, sol::lua_nil_t{});
        }
        return sol::make_object(lua, arch->GetFlagName(idx));
    }
    if (tag == "reg_stack") {
        Ref<Architecture> arch = ArchFor(instr);
        uint32_t idx = instr.GetRawOperandAsRegister(slot);
        if (!arch || idx == kNoRegisterSentinel) {
            return sol::make_object(lua, sol::lua_nil_t{});
        }
        return sol::make_object(lua, arch->GetRegisterStackName(idx));
    }
    if (tag == "sem_class") {
        Ref<Architecture> arch = ArchFor(instr);
        uint32_t idx = instr.GetRawOperandAsRegister(slot);
        // python/lowlevelil.py:1089: sem_class idx == 0 is None.
        if (!arch || idx == 0) {
            return sol::make_object(lua, sol::lua_nil_t{});
        }
        return sol::make_object(lua,
            arch->GetSemanticFlagClassName(idx));
    }
    if (tag == "sem_group") {
        Ref<Architecture> arch = ArchFor(instr);
        uint32_t idx = instr.GetRawOperandAsRegister(slot);
        if (!arch || idx == kNoRegisterSentinel) {
            return sol::make_object(lua, sol::lua_nil_t{});
        }
        return sol::make_object(lua,
            arch->GetSemanticFlagGroupName(idx));
    }
    if (tag == "intrinsic") {
        Ref<Architecture> arch = ArchFor(instr);
        uint32_t idx = instr.GetRawOperandAsRegister(slot);
        if (!arch || idx == kNoRegisterSentinel) {
            return sol::make_object(lua, sol::lua_nil_t{});
        }
        return sol::make_object(lua, arch->GetIntrinsicName(idx));
    }
    if (tag == "cond") {
        BNLowLevelILFlagCondition c =
            instr.GetRawOperandAsFlagCondition(slot);
        return sol::make_object(lua, std::string(EnumToString(c)));
    }
    if (tag == "target_map") {
        sol::table t = lua.create_table();
        std::map<uint64_t, uint64_t> m =
            instr.GetRawOperandAsIndexMap(slot);
        for (const auto& entry : m) {
            t[static_cast<lua_Integer>(entry.first)] =
                static_cast<lua_Integer>(entry.second);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "reg_stack_adjust") {
        sol::table t = lua.create_table();
        std::map<uint32_t, int32_t> adjusts =
            instr.GetRawOperandAsRegisterStackAdjustments(slot);
        Ref<Architecture> arch = ArchFor(instr);
        if (arch) {
            for (const auto& entry : adjusts) {
                t[arch->GetRegisterStackName(entry.first)] =
                    static_cast<lua_Integer>(entry.second);
            }
        }
        return sol::make_object(lua, t);
    }
    if (tag == "reg_ssa") {
        SSARegister ssa = instr.GetRawOperandAsSSARegister(slot);
        Ref<Architecture> arch = ArchFor(instr);
        std::string n;
        if (arch && ssa.reg != kNoRegisterSentinel) {
            n = arch->GetRegisterName(ssa.reg);
        }
        return sol::make_object(lua,
            MakeSSAEntry(lua, "reg", n, ssa.version));
    }
    if (tag == "reg_stack_ssa") {
        SSARegisterStack ssa =
            instr.GetRawOperandAsSSARegisterStack(slot);
        Ref<Architecture> arch = ArchFor(instr);
        std::string n;
        if (arch && ssa.regStack != kNoRegisterSentinel) {
            n = arch->GetRegisterStackName(ssa.regStack);
        }
        return sol::make_object(lua,
            MakeSSAEntry(lua, "reg_stack", n, ssa.version));
    }
    if (tag == "reg_stack_ssa_dest_and_src") {
        SSARegisterStack src =
            instr.GetRawOperandAsPartialSSARegisterStackSource(slot);
        sol::table t = lua.create_table(0, 3);
        Ref<Architecture> arch = ArchFor(instr);
        if (arch && src.regStack != kNoRegisterSentinel) {
            t["reg_stack"] = arch->GetRegisterStackName(src.regStack);
        } else {
            t["reg_stack"] = sol::lua_nil_t{};
        }
        t["version"] = static_cast<lua_Integer>(src.version);
        t["source_version"] = static_cast<lua_Integer>(src.version);
        return sol::make_object(lua, t);
    }
    if (tag == "flag_ssa") {
        SSAFlag ssa = instr.GetRawOperandAsSSAFlag(slot);
        Ref<Architecture> arch = ArchFor(instr);
        std::string n;
        if (arch && ssa.flag != kNoRegisterSentinel) {
            n = arch->GetFlagName(ssa.flag);
        }
        return sol::make_object(lua,
            MakeSSAEntry(lua, "flag", n, ssa.version));
    }
    if (tag == "reg_ssa_list") {
        sol::table t = lua.create_table();
        int i = 1;
        Ref<Architecture> arch = ArchFor(instr);
        for (auto ssa : instr.GetRawOperandAsSSARegisterList(slot)) {
            std::string n;
            if (arch && ssa.reg != kNoRegisterSentinel) {
                n = arch->GetRegisterName(ssa.reg);
            }
            t[i++] = MakeSSAEntry(lua, "reg", n, ssa.version);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "reg_stack_ssa_list") {
        sol::table t = lua.create_table();
        int i = 1;
        Ref<Architecture> arch = ArchFor(instr);
        for (auto ssa :
             instr.GetRawOperandAsSSARegisterStackList(slot)) {
            std::string n;
            if (arch && ssa.regStack != kNoRegisterSentinel) {
                n = arch->GetRegisterStackName(ssa.regStack);
            }
            t[i++] = MakeSSAEntry(lua, "reg_stack", n, ssa.version);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "flag_ssa_list") {
        sol::table t = lua.create_table();
        int i = 1;
        Ref<Architecture> arch = ArchFor(instr);
        for (auto ssa : instr.GetRawOperandAsSSAFlagList(slot)) {
            std::string n;
            if (arch && ssa.flag != kNoRegisterSentinel) {
                n = arch->GetFlagName(ssa.flag);
            }
            t[i++] = MakeSSAEntry(lua, "flag", n, ssa.version);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "reg_or_flag_list") {
        // Discriminated table per py-researcher-2 recommendation:
        // {kind = "reg" | "flag", name = <str>} so scripts can tell
        // register vs flag slots apart without re-resolving by name.
        sol::table t = lua.create_table();
        int i = 1;
        Ref<Architecture> arch = ArchFor(instr);
        for (auto rf :
             instr.GetRawOperandAsRegisterOrFlagList(slot)) {
            sol::table entry = lua.create_table(0, 2);
            if (!arch) {
                entry["kind"] = rf.isFlag ? "flag" : "reg";
                entry["name"] = sol::lua_nil_t{};
            } else if (rf.isFlag) {
                entry["kind"] = "flag";
                entry["name"] = arch->GetFlagName(rf.index);
            } else {
                entry["kind"] = "reg";
                entry["name"] = arch->GetRegisterName(rf.index);
            }
            t[i++] = entry;
        }
        return sol::make_object(lua, t);
    }
    if (tag == "reg_or_flag_ssa_list") {
        sol::table t = lua.create_table();
        int i = 1;
        Ref<Architecture> arch = ArchFor(instr);
        for (auto rf :
             instr.GetRawOperandAsSSARegisterOrFlagList(slot)) {
            sol::table entry = lua.create_table(0, 3);
            if (!arch) {
                entry["kind"] = rf.regOrFlag.isFlag ? "flag" : "reg";
                entry["name"] = sol::lua_nil_t{};
            } else if (rf.regOrFlag.isFlag) {
                entry["kind"] = "flag";
                entry["name"] = arch->GetFlagName(rf.regOrFlag.index);
            } else {
                entry["kind"] = "reg";
                entry["name"] = arch->GetRegisterName(rf.regOrFlag.index);
            }
            entry["version"] = static_cast<lua_Integer>(rf.version);
            t[i++] = entry;
        }
        return sol::make_object(lua, t);
    }
    if (tag == "constraint") {
        // R9.1 stub per docs/il-metatable-design.md section 2c.
        // Full PossibleValueSet projection defers to the dataflow wave.
        sol::table t = lua.create_table(0, 2);
        t["type"] = "constraint";
        t["repr"] = "PossibleValueSet";
        return sol::make_object(lua, t);
    }
    if (tag == "unknown") {
        return ProjectUnknownField(lua, instr, spec);
    }

    // Generator should have covered every tag; a miss returns nil so
    // scripts can detect the gap rather than crashing.
    return sol::make_object(lua, sol::lua_nil_t{});
}

sol::table BuildLLILOperandsTable(sol::this_state ts,
                                    const LowLevelILInstruction& instr) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    const std::vector<LLILOperandSpec>& specs =
        LLILOperandSpecsForOperation(instr.operation);
    int i = 1;
    for (const LLILOperandSpec& spec : specs) {
        out[i++] = LLILOperandToLua(lua, instr, spec);
    }
    return out;
}

sol::table BuildLLILDetailedOperandsTable(
    sol::this_state ts, const LowLevelILInstruction& instr) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    const std::vector<LLILOperandSpec>& specs =
        LLILOperandSpecsForOperation(instr.operation);
    int i = 1;
    for (const LLILOperandSpec& spec : specs) {
        sol::table entry = lua.create_table(0, 3);
        entry["name"] = std::string(spec.name ? spec.name : "");
        entry["type"] = std::string(spec.type_tag ? spec.type_tag : "");
        entry["value"] = LLILOperandToLua(lua, instr, spec);
        out[i++] = entry;
    }
    return out;
}

namespace {

void AppendPrefixOperands(sol::state_view lua, sol::table& out, int& idx,
                           const LowLevelILInstruction& instr) {
    sol::table marker = lua.create_table(0, 2);
    marker["operation"] = std::string(EnumToString(instr.operation));
    marker["size"] = static_cast<lua_Integer>(instr.size);
    out[idx++] = marker;

    const std::vector<LLILOperandSpec>& specs =
        LLILOperandSpecsForOperation(instr.operation);
    for (const LLILOperandSpec& spec : specs) {
        const std::string tag = spec.type_tag ? spec.type_tag : "";
        if (tag == "expr") {
            LowLevelILInstruction nested =
                instr.GetRawOperandAsExpr(spec.slot_first);
            AppendPrefixOperands(lua, out, idx, nested);
        } else if (tag == "expr_list") {
            for (auto nested :
                 instr.GetRawOperandAsExprList(spec.slot_first)) {
                AppendPrefixOperands(lua, out, idx, nested);
            }
        } else {
            out[idx++] = LLILOperandToLua(lua, instr, spec);
        }
    }
}

}  // namespace

sol::table BuildLLILPrefixOperandsTable(
    sol::this_state ts, const LowLevelILInstruction& instr) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    int idx = 1;
    AppendPrefixOperands(lua, out, idx, instr);
    return out;
}

namespace {

void TraverseRecursive(sol::state_view lua, sol::table& results, int& idx,
                        const LowLevelILInstruction& instr,
                        sol::function& cb) {
    sol::protected_function_result rv = cb(instr);
    if (rv.valid()) {
        sol::object result = rv;
        if (result.get_type() != sol::type::nil) {
            results[idx++] = result;
        }
    }
    const std::vector<LLILOperandSpec>& specs =
        LLILOperandSpecsForOperation(instr.operation);
    for (const LLILOperandSpec& spec : specs) {
        const std::string tag = spec.type_tag ? spec.type_tag : "";
        if (tag == "expr") {
            LowLevelILInstruction nested =
                instr.GetRawOperandAsExpr(spec.slot_first);
            TraverseRecursive(lua, results, idx, nested, cb);
        } else if (tag == "expr_list") {
            for (auto nested :
                 instr.GetRawOperandAsExprList(spec.slot_first)) {
                TraverseRecursive(lua, results, idx, nested, cb);
            }
        }
    }
}

}  // namespace

sol::table TraverseLLILInstruction(sol::this_state ts,
                                    const LowLevelILInstruction& instr,
                                    sol::function cb) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    int idx = 1;
    TraverseRecursive(lua, out, idx, instr, cb);
    return out;
}

// ============================================================
// MLIL projection (R9.2 commit B).
// ============================================================

Ref<Architecture> ArchFor(const MediumLevelILInstruction& instr) {
    if (!instr.function) return Ref<Architecture>();
    Ref<Function> f = instr.function->GetFunction();
    if (!f) return Ref<Architecture>();
    return f->GetArchitecture();
}

namespace {

// Translate a BN Variable into the canonical {source_type, index,
// storage} table shape already used by bindings/variable.cpp for
// Function.vars / Variable usertypes. See docs/il-metatable-design.md
// section 12.3.
sol::table VariableToTable(sol::state_view lua, const Variable& var) {
    sol::table t = lua.create_table(0, 3);
    t["source_type"] = std::string(
        EnumToString(static_cast<BNVariableSourceType>(var.type)));
    t["index"] = static_cast<lua_Integer>(var.index);
    t["storage"] = static_cast<lua_Integer>(var.storage);
    return t;
}

// Translate a BN SSAVariable into {var = <var-table>, version = N}.
sol::table SSAVariableToTable(sol::state_view lua,
                               const SSAVariable& ssa) {
    sol::table t = lua.create_table(0, 2);
    t["var"] = VariableToTable(lua, ssa.var);
    t["version"] = static_cast<lua_Integer>(ssa.version);
    return t;
}

// Translate a BN ConstantData into {state, value, size}. R9.2 leaves
// the actual byte-buffer materialization to the dataflow wave; the
// stub surface mirrors the CamelCase tag in the operand table and
// lets scripts detect the state (via the shared
// EnumToString<BNRegisterValueType> helper in common.h).
sol::table ConstantDataToTable(sol::state_view lua,
                                const ConstantData& cd) {
    sol::table t = lua.create_table(0, 3);
    t["state"] = std::string(EnumToString(cd.state));
    t["value"] = static_cast<lua_Integer>(cd.value);
    t["size"] = static_cast<lua_Integer>(cd.size);
    return t;
}

// Project an expr_list slot to a 1-indexed Lua sequence of nested
// MLIL instructions. Parallel to the LLIL helper above.
sol::table ProjectMLILExprList(sol::state_view lua,
                                 const MediumLevelILInstruction& instr,
                                 size_t slot) {
    sol::table t = lua.create_table();
    int i = 1;
    auto list = instr.GetRawOperandAsExprList(slot);
    for (size_t k = 0; k < list.size(); ++k) {
        t[i++] = list[k];
    }
    return t;
}

}  // namespace

sol::object MLILOperandToLua(sol::state_view lua,
                              const MediumLevelILInstruction& instr,
                              const MLILOperandSpec& spec) {
    const std::string tag = spec.type_tag ? spec.type_tag : "";
    const size_t slot = spec.slot_first;

    if (tag == "int") {
        uint64_t raw = instr.GetRawOperandAsInteger(slot);
        return sol::make_object(lua,
            static_cast<lua_Integer>(static_cast<int64_t>(raw)));
    }
    if (tag == "float") {
        uint64_t raw = instr.GetRawOperandAsInteger(slot);
        double d = 0.0;
        if (instr.size == 4) {
            float f = 0.0f;
            uint32_t bits = static_cast<uint32_t>(raw & 0xffffffffu);
            std::memcpy(&f, &bits, sizeof(f));
            d = static_cast<double>(f);
        } else {
            std::memcpy(&d, &raw, sizeof(d));
        }
        return sol::make_object(lua, d);
    }
    if (tag == "expr") {
        return sol::make_object(lua, instr.GetRawOperandAsExpr(slot));
    }
    if (tag == "expr_list") {
        return sol::make_object(lua, ProjectMLILExprList(lua, instr, slot));
    }
    if (tag == "int_list") {
        sol::table t = lua.create_table();
        int i = 1;
        auto list = instr.GetRawOperandAsIndexList(slot);
        for (size_t k = 0; k < list.size(); ++k) {
            t[i++] = static_cast<lua_Integer>(list[k]);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "target_map") {
        sol::table t = lua.create_table();
        std::map<uint64_t, size_t> m =
            instr.GetRawOperandAsIndexMap(slot);
        for (const auto& entry : m) {
            t[static_cast<lua_Integer>(entry.first)] =
                static_cast<lua_Integer>(entry.second);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "intrinsic") {
        Ref<Architecture> arch = ArchFor(instr);
        uint32_t idx = static_cast<uint32_t>(
            instr.GetRawOperandAsInteger(slot) & 0xffffffffu);
        if (!arch || idx == 0xffffffffu) {
            return sol::make_object(lua, sol::lua_nil_t{});
        }
        return sol::make_object(lua, arch->GetIntrinsicName(idx));
    }
    if (tag == "cond") {
        // Shared with LLIL: MLIL uses the same BNLowLevelILFlagCondition
        // vocabulary for flag-condition slots.
        BNLowLevelILFlagCondition c =
            static_cast<BNLowLevelILFlagCondition>(
                instr.GetRawOperandAsInteger(slot));
        return sol::make_object(lua, std::string(EnumToString(c)));
    }
    if (tag == "var") {
        Variable v = instr.GetRawOperandAsVariable(slot);
        return sol::make_object(lua, VariableToTable(lua, v));
    }
    if (tag == "var_ssa") {
        SSAVariable ssa = instr.GetRawOperandAsSSAVariable(slot);
        return sol::make_object(lua, SSAVariableToTable(lua, ssa));
    }
    if (tag == "var_list") {
        sol::table t = lua.create_table();
        int i = 1;
        auto list = instr.GetRawOperandAsVariableList(slot);
        for (size_t k = 0; k < list.size(); ++k) {
            t[i++] = VariableToTable(lua, list[k]);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "var_ssa_list") {
        sol::table t = lua.create_table();
        int i = 1;
        auto list = instr.GetRawOperandAsSSAVariableList(slot);
        for (size_t k = 0; k < list.size(); ++k) {
            t[i++] = SSAVariableToTable(lua, list[k]);
        }
        return sol::make_object(lua, t);
    }
    if (tag == "var_ssa_dest_and_src") {
        // GetRawOperandAsPartialSSAVariableSource consumes the
        // (var_identifier, src_version) pair at slot and slot+1. The
        // generator emits the spec with slot_last == slot_first + 1
        // or slot_first + 2 depending on where the src version lives
        // per the Python accessor; BN's internal helper reads both
        // halves from the base slot.
        SSAVariable ssa =
            instr.GetRawOperandAsPartialSSAVariableSource(slot);
        return sol::make_object(lua, SSAVariableToTable(lua, ssa));
    }
    if (tag == "ConstantData") {
        ConstantData cd = instr.GetRawOperandAsConstantData(slot);
        return sol::make_object(lua, ConstantDataToTable(lua, cd));
    }

    // Unknown tag (includes LLIL-only tags that generators should
    // never emit for MLIL). Return nil so scripts can detect the gap.
    return sol::make_object(lua, sol::lua_nil_t{});
}

sol::table BuildMLILOperandsTable(sol::this_state ts,
                                    const MediumLevelILInstruction& instr) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    const std::vector<MLILOperandSpec>& specs =
        MLILOperandSpecsForOperation(instr.operation);
    int i = 1;
    for (const MLILOperandSpec& spec : specs) {
        out[i++] = MLILOperandToLua(lua, instr, spec);
    }
    return out;
}

sol::table BuildMLILDetailedOperandsTable(
    sol::this_state ts, const MediumLevelILInstruction& instr) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    const std::vector<MLILOperandSpec>& specs =
        MLILOperandSpecsForOperation(instr.operation);
    int i = 1;
    for (const MLILOperandSpec& spec : specs) {
        sol::table entry = lua.create_table(0, 3);
        entry["name"] = std::string(spec.name ? spec.name : "");
        entry["type"] = std::string(spec.type_tag ? spec.type_tag : "");
        entry["value"] = MLILOperandToLua(lua, instr, spec);
        out[i++] = entry;
    }
    return out;
}

namespace {

void AppendMLILPrefixOperands(sol::state_view lua, sol::table& out,
                                int& idx,
                                const MediumLevelILInstruction& instr) {
    sol::table marker = lua.create_table(0, 2);
    marker["operation"] = std::string(EnumToString(instr.operation));
    marker["size"] = static_cast<lua_Integer>(instr.size);
    out[idx++] = marker;

    const std::vector<MLILOperandSpec>& specs =
        MLILOperandSpecsForOperation(instr.operation);
    for (const MLILOperandSpec& spec : specs) {
        const std::string tag = spec.type_tag ? spec.type_tag : "";
        if (tag == "expr") {
            MediumLevelILInstruction nested =
                instr.GetRawOperandAsExpr(spec.slot_first);
            AppendMLILPrefixOperands(lua, out, idx, nested);
        } else if (tag == "expr_list") {
            auto list = instr.GetRawOperandAsExprList(spec.slot_first);
            for (size_t k = 0; k < list.size(); ++k) {
                MediumLevelILInstruction nested = list[k];
                AppendMLILPrefixOperands(lua, out, idx, nested);
            }
        } else {
            out[idx++] = MLILOperandToLua(lua, instr, spec);
        }
    }
}

}  // namespace

sol::table BuildMLILPrefixOperandsTable(
    sol::this_state ts, const MediumLevelILInstruction& instr) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    int idx = 1;
    AppendMLILPrefixOperands(lua, out, idx, instr);
    return out;
}

namespace {

void TraverseMLILRecursive(sol::state_view lua, sol::table& results,
                             int& idx,
                             const MediumLevelILInstruction& instr,
                             sol::function& cb) {
    sol::protected_function_result rv = cb(instr);
    if (rv.valid()) {
        sol::object result = rv;
        if (result.get_type() != sol::type::nil) {
            results[idx++] = result;
        }
    }
    const std::vector<MLILOperandSpec>& specs =
        MLILOperandSpecsForOperation(instr.operation);
    for (const MLILOperandSpec& spec : specs) {
        const std::string tag = spec.type_tag ? spec.type_tag : "";
        if (tag == "expr") {
            MediumLevelILInstruction nested =
                instr.GetRawOperandAsExpr(spec.slot_first);
            TraverseMLILRecursive(lua, results, idx, nested, cb);
        } else if (tag == "expr_list") {
            auto list = instr.GetRawOperandAsExprList(spec.slot_first);
            for (size_t k = 0; k < list.size(); ++k) {
                MediumLevelILInstruction nested = list[k];
                TraverseMLILRecursive(lua, results, idx, nested, cb);
            }
        }
    }
}

}  // namespace

sol::table TraverseMLILInstruction(sol::this_state ts,
                                    const MediumLevelILInstruction& instr,
                                    sol::function cb) {
    sol::state_view lua(ts);
    sol::table out = lua.create_table();
    int idx = 1;
    TraverseMLILRecursive(lua, out, idx, instr, cb);
    return out;
}

}  // namespace BinjaLua
