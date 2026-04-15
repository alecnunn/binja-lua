// Sol2 CallingConvention bindings for binja-lua
// Read-only binding for BinaryNinja::CallingConvention (R5). Mirrors
// the handle-initialised field set on the Python side at
// python/callingconvention.py:122-224.
//
// Deferred (do NOT bind in R5, see python/callingconvention.py:526-570):
//   - GetIncomingRegisterValue / GetIncomingFlagValue
//   - GetIncomingVariableForParameterVariable
//   - GetParameterVariableForIncomingVariable
// These round-trip RegisterValue / Variable types whose Lua wrappers
// are not yet comprehensive enough to support a clean binding. Revisit
// once those usertypes are audited.
//
// Also not exposed: `confidence` / `with_confidence`. The C++ API has
// no confidence field on CallingConvention itself; Python synthesises
// it from a constructor kwarg and only Confidence<Ref<CallingConvention>>
// (tracked by callers like Function) actually carries the confidence
// value. Exposing a Lua-side value would diverge silently from any
// round-trip through C++, so it is skipped.

#include "common.h"

#include <cstdint>
#include <string>
#include <vector>

namespace BinjaLua {

namespace {

constexpr uint32_t kNoRegister = 0xffffffffu;

// Project a register index held by the CallingConvention through its
// bound Architecture's GetRegisterName, mapping the kNoRegister
// sentinel to Lua nil per python/callingconvention.py:193-214.
sol::object ReturnRegOrNil(sol::this_state ts, CallingConvention& cc,
                           uint32_t reg) {
    sol::state_view lua(ts);
    if (reg == kNoRegister) {
        return sol::make_object(lua, sol::nil);
    }
    Ref<Architecture> arch = cc.GetArchitecture();
    if (!arch) {
        return sol::make_object(lua, sol::nil);
    }
    return sol::make_object(lua, arch->GetRegisterName(reg));
}

// Map a list of register indices through the bound architecture's name
// table. Matches the per-list loops at
// python/callingconvention.py:132-224 that call `arch.get_reg_name(idx)`
// on every entry.
sol::table RegIndicesToNameTable(sol::state_view lua, CallingConvention& cc,
                                 const std::vector<uint32_t>& regs) {
    Ref<Architecture> arch = cc.GetArchitecture();
    sol::table result = lua.create_table(static_cast<int>(regs.size()), 0);
    int idx = 1;
    for (uint32_t reg : regs) {
        if (!arch) continue;
        std::string name = arch->GetRegisterName(reg);
        if (name.empty()) continue;
        result[idx++] = name;
    }
    return result;
}

}  // namespace

void RegisterCallingConventionBindings(sol::state_view lua,
                                        Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering CallingConvention bindings");

    lua.new_usertype<CallingConvention>(CALLINGCONVENTION_METATABLE,
        sol::no_constructor,

        // Identity

        "name", sol::property([](CallingConvention& cc) -> std::string {
            return cc.GetName();
        }),

        "arch", sol::property([](CallingConvention& cc) -> Ref<Architecture> {
            return cc.GetArchitecture();
        }),

        // Register sets (arrays of name strings). Bound as methods
        // (colon-call on the Lua side) instead of sol::property because
        // sol2 3.3.0 crashes on MSVC when sol::property is combined
        // with sol::this_state; see task #12 for the architecture.cpp
        // repro and task #14 for the same fix here.

        "caller_saved_regs", [](sol::this_state ts, CallingConvention& cc)
                                  -> sol::table {
            return RegIndicesToNameTable(sol::state_view(ts), cc,
                                         cc.GetCallerSavedRegisters());
        },

        "callee_saved_regs", [](sol::this_state ts, CallingConvention& cc)
                                  -> sol::table {
            return RegIndicesToNameTable(sol::state_view(ts), cc,
                                         cc.GetCalleeSavedRegisters());
        },

        "int_arg_regs", [](sol::this_state ts, CallingConvention& cc)
                             -> sol::table {
            return RegIndicesToNameTable(sol::state_view(ts), cc,
                                         cc.GetIntegerArgumentRegisters());
        },

        "float_arg_regs", [](sol::this_state ts, CallingConvention& cc)
                               -> sol::table {
            return RegIndicesToNameTable(sol::state_view(ts), cc,
                                         cc.GetFloatArgumentRegisters());
        },

        "required_arg_regs", [](sol::this_state ts, CallingConvention& cc)
                                  -> sol::table {
            return RegIndicesToNameTable(sol::state_view(ts), cc,
                                         cc.GetRequiredArgumentRegisters());
        },

        "required_clobbered_regs",
        [](sol::this_state ts, CallingConvention& cc) -> sol::table {
            return RegIndicesToNameTable(
                sol::state_view(ts), cc,
                cc.GetRequiredClobberedRegisters());
        },

        "implicitly_defined_regs",
        [](sol::this_state ts, CallingConvention& cc) -> sol::table {
            return RegIndicesToNameTable(
                sol::state_view(ts), cc,
                cc.GetImplicitlyDefinedRegisters());
        },

        // Return / special single-register accessors. Apply the
        // 0xffffffff -> nil sentinel rule per
        // python/callingconvention.py:192-214. Same method-form
        // workaround as above.

        "int_return_reg", [](sol::this_state ts, CallingConvention& cc)
                               -> sol::object {
            return ReturnRegOrNil(ts, cc,
                                  cc.GetIntegerReturnValueRegister());
        },

        "high_int_return_reg", [](sol::this_state ts, CallingConvention& cc)
                                    -> sol::object {
            return ReturnRegOrNil(ts, cc,
                                  cc.GetHighIntegerReturnValueRegister());
        },

        "float_return_reg", [](sol::this_state ts, CallingConvention& cc)
                                 -> sol::object {
            return ReturnRegOrNil(ts, cc,
                                  cc.GetFloatReturnValueRegister());
        },

        "global_pointer_reg", [](sol::this_state ts, CallingConvention& cc)
                                   -> sol::object {
            return ReturnRegOrNil(ts, cc, cc.GetGlobalPointerRegister());
        },

        // Boolean heuristics flags. Names match Python verbatim at
        // python/callingconvention.py:126-130.

        "arg_regs_share_index", sol::property([](CallingConvention& cc)
                                                   -> bool {
            return cc.AreArgumentRegistersSharedIndex();
        }),

        "arg_regs_for_varargs", sol::property([](CallingConvention& cc)
                                                   -> bool {
            return cc.AreArgumentRegistersUsedForVarArgs();
        }),

        "stack_reserved_for_arg_regs", sol::property([](CallingConvention& cc)
                                                          -> bool {
            return cc.IsStackReservedForArgumentRegisters();
        }),

        "stack_adjusted_on_return", sol::property([](CallingConvention& cc)
                                                       -> bool {
            return cc.IsStackAdjustedOnReturn();
        }),

        "eligible_for_heuristics", sol::property([](CallingConvention& cc)
                                                      -> bool {
            return cc.IsEligibleForHeuristics();
        }),

        // Equality by underlying BN object pointer.
        sol::meta_function::equal_to, [](CallingConvention& lhs,
                                         CallingConvention& rhs) -> bool {
            return CallingConvention::GetObject(&lhs) ==
                   CallingConvention::GetObject(&rhs);
        },

        sol::meta_function::to_string, [](CallingConvention& cc)
                                           -> std::string {
            Ref<Architecture> arch = cc.GetArchitecture();
            return fmt::format("<CallingConvention: {} {}>",
                               arch ? arch->GetName() : "<unknown>",
                               cc.GetName());
        }
    );

    if (logger) logger->LogDebug("CallingConvention bindings registered");
}

}  // namespace BinjaLua
