// Sol2 Architecture bindings for binja-lua
// Read-only binding for BinaryNinja::CoreArchitecture (R4).
// Plugin-authoring (custom architecture subclasses), assembly, patch
// helpers, LLIL emission for flags, intrinsics, and register stacks are
// deliberately out of scope. Intrinsics and standalone_platform ship in
// later roadmap tasks.

#include "common.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace BinjaLua {

namespace {

// Sentinel used by Binary Ninja for "no such register" on single-register
// properties like link_reg and the CallingConvention int/float return
// registers. Matches python/architecture.py:2702-2706.
constexpr uint32_t kNoRegister = 0xffffffffu;

// Project a single register index through Architecture::GetRegisterName,
// mapping the kNoRegister sentinel to an empty optional (-> Lua nil).
sol::object RegisterNameOrNil(sol::this_state ts, Architecture& arch,
                              uint32_t reg) {
    sol::state_view lua(ts);
    if (reg == kNoRegister) {
        return sol::make_object(lua, sol::nil);
    }
    return sol::make_object(lua, arch.GetRegisterName(reg));
}

// Build a Lua table of register-name strings from a list of register
// indices. Names that fail to resolve are skipped rather than surfaced
// as empty strings; a register index we cannot resolve is a BN bug, not
// something Lua scripts should have to filter out.
sol::table RegisterNamesToTable(sol::state_view lua, Architecture& arch,
                                const std::vector<uint32_t>& regs) {
    sol::table result = lua.create_table(static_cast<int>(regs.size()), 0);
    int idx = 1;
    for (uint32_t reg : regs) {
        std::string name = arch.GetRegisterName(reg);
        if (name.empty()) continue;
        result[idx++] = name;
    }
    return result;
}

// Build a Lua table describing a single register, matching the Python
// dataclass RegisterInfo at python/architecture.py:537.
sol::table RegisterInfoToTable(sol::state_view lua, Architecture& arch,
                               uint32_t reg) {
    BNRegisterInfo info = arch.GetRegisterInfo(reg);
    sol::table t = lua.create_table(0, 5);
    t["full_width_reg"] = arch.GetRegisterName(info.fullWidthRegister);
    t["size"]           = info.size;
    t["offset"]         = info.offset;
    t["extend"]         = EnumToString(info.extend);
    t["index"]          = reg;
    return t;
}

// Build a Lua table matching the Python InstructionInfo dataclass at
// python/architecture.py:598, with branches flattened into a list of
// plain tables matching InstructionBranch at :586.
sol::table InstructionInfoToTable(sol::state_view lua,
                                  const InstructionInfo& info) {
    sol::table t = lua.create_table(0, 4);
    t["length"]                         = info.length;
    t["arch_transition_by_target_addr"] = info.archTransitionByTargetAddr;
    t["branch_delay"]                   = info.delaySlots;

    sol::table branches =
        lua.create_table(static_cast<int>(info.branchCount), 0);
    for (size_t i = 0; i < info.branchCount; ++i) {
        sol::table branch = lua.create_table(0, 3);
        branch["type"]   = EnumToString(info.branchType[i]);
        branch["target"] = HexAddress(info.branchTarget[i]);
        if (info.branchArch[i]) {
            Ref<Architecture> branch_arch = new CoreArchitecture(
                info.branchArch[i]);
            branch["arch"] = branch_arch;
        }
        branches[i + 1] = branch;
    }
    t["branches"] = branches;
    return t;
}

// Convert a Binary Ninja InstructionTextToken vector into a Lua table
// matching the shape produced elsewhere in the bindings: a sequence of
// {text, type, value} entries keyed by index. This mirrors the shape
// that BasicBlock:disassembly already emits so callers that already
// know how to consume it keep working here.
sol::table InstructionTextTokensToTable(
    sol::state_view lua, const std::vector<InstructionTextToken>& tokens) {
    sol::table result = lua.create_table(static_cast<int>(tokens.size()), 0);
    for (size_t i = 0; i < tokens.size(); ++i) {
        sol::table tok = lua.create_table(0, 3);
        tok["text"]  = tokens[i].text;
        tok["type"]  = static_cast<int>(tokens[i].type);
        tok["value"] = tokens[i].value;
        result[i + 1] = tok;
    }
    return result;
}

}  // namespace

void RegisterArchitectureBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Architecture bindings");

    lua.new_usertype<Architecture>(ARCHITECTURE_METATABLE,
        sol::no_constructor,

        // Basic identity

        "name", sol::property([](Architecture& a) -> std::string {
            return a.GetName();
        }),

        "endianness", sol::property([](Architecture& a) -> std::string {
            return EnumToString(a.GetEndianness());
        }),

        "address_size", sol::property([](Architecture& a) -> size_t {
            return a.GetAddressSize();
        }),

        "default_int_size", sol::property([](Architecture& a) -> size_t {
            return a.GetDefaultIntegerSize();
        }),

        "instr_alignment", sol::property([](Architecture& a) -> size_t {
            return a.GetInstructionAlignment();
        }),

        // Note: this is the plugin decode-buffer size, NOT the CPU's
        // architectural maximum instruction length.
        "max_instr_length", sol::property([](Architecture& a) -> size_t {
            return a.GetMaxInstructionLength();
        }),

        "opcode_display_length", sol::property([](Architecture& a) -> size_t {
            return a.GetOpcodeDisplayLength();
        }),

        "stack_pointer", sol::property([](Architecture& a) -> std::string {
            return a.GetRegisterName(a.GetStackPointerRegister());
        }),

        // sol::property with sol::this_state is known to crash sol2 3.3.0
        // under MSVC (see bindings/binaryview.cpp:45 note, confirmed by
        // the task #12 repro). Every accessor below that needs the Lua
        // state to build a table is therefore bound as a plain method
        // (colon-call on the Lua side) instead of a property. link_reg
        // is a method too for consistency with the table accessors.

        "link_reg", [](sol::this_state ts, Architecture& a) -> sol::object {
            return RegisterNameOrNil(ts, a, a.GetLinkRegister());
        },

        "can_assemble", sol::property([](Architecture& a) -> bool {
            return a.CanAssemble();
        }),

        "standalone_platform",
        sol::property([](Architecture& a) -> Ref<Platform> {
            return a.GetStandalonePlatform();
        }),

        // Register catalog (bound as methods per the note above)

        "regs", [](sol::this_state ts, Architecture& a) -> sol::table {
            sol::state_view l(ts);
            std::vector<uint32_t> regs = a.GetAllRegisters();
            sol::table out = l.create_table(0, static_cast<int>(regs.size()));
            for (uint32_t reg : regs) {
                std::string name = a.GetRegisterName(reg);
                if (name.empty()) continue;
                out[name] = RegisterInfoToTable(l, a, reg);
            }
            return out;
        },

        "full_width_regs", [](sol::this_state ts, Architecture& a)
                                -> sol::table {
            return RegisterNamesToTable(sol::state_view(ts), a,
                                        a.GetFullWidthRegisters());
        },

        "global_regs", [](sol::this_state ts, Architecture& a) -> sol::table {
            return RegisterNamesToTable(sol::state_view(ts), a,
                                        a.GetGlobalRegisters());
        },

        "system_regs", [](sol::this_state ts, Architecture& a) -> sol::table {
            return RegisterNamesToTable(sol::state_view(ts), a,
                                        a.GetSystemRegisters());
        },

        "get_reg_index", [](Architecture& a, const std::string& name)
                             -> uint32_t {
            return a.GetRegisterByName(name);
        },

        "get_reg_name", [](Architecture& a, uint32_t index) -> std::string {
            return a.GetRegisterName(index);
        },

        // Flags (bound as methods per the note above)

        "flags", [](sol::this_state ts, Architecture& a) -> sol::table {
            sol::state_view l(ts);
            std::vector<uint32_t> flags = a.GetAllFlags();
            sol::table out = l.create_table(static_cast<int>(flags.size()), 0);
            int idx = 1;
            for (uint32_t flag : flags) {
                std::string name = a.GetFlagName(flag);
                if (name.empty()) continue;
                out[idx++] = name;
            }
            return out;
        },

        "flag_write_types", [](sol::this_state ts, Architecture& a)
                                 -> sol::table {
            sol::state_view l(ts);
            std::vector<uint32_t> types = a.GetAllFlagWriteTypes();
            sol::table out = l.create_table(static_cast<int>(types.size()), 0);
            int idx = 1;
            for (uint32_t t : types) {
                std::string name = a.GetFlagWriteTypeName(t);
                if (name.empty()) continue;
                out[idx++] = name;
            }
            return out;
        },

        "semantic_flag_classes", [](sol::this_state ts, Architecture& a)
                                      -> sol::table {
            sol::state_view l(ts);
            std::vector<uint32_t> classes = a.GetAllSemanticFlagClasses();
            sol::table out =
                l.create_table(static_cast<int>(classes.size()), 0);
            int idx = 1;
            for (uint32_t c : classes) {
                std::string name = a.GetSemanticFlagClassName(c);
                if (name.empty()) continue;
                out[idx++] = name;
            }
            return out;
        },

        "semantic_flag_groups", [](sol::this_state ts, Architecture& a)
                                     -> sol::table {
            sol::state_view l(ts);
            std::vector<uint32_t> groups = a.GetAllSemanticFlagGroups();
            sol::table out =
                l.create_table(static_cast<int>(groups.size()), 0);
            int idx = 1;
            for (uint32_t g : groups) {
                std::string name = a.GetSemanticFlagGroupName(g);
                if (name.empty()) continue;
                out[idx++] = name;
            }
            return out;
        },

        "flag_roles", [](sol::this_state ts, Architecture& a) -> sol::table {
            sol::state_view l(ts);
            std::vector<uint32_t> flags = a.GetAllFlags();
            sol::table out = l.create_table(0, static_cast<int>(flags.size()));
            for (uint32_t flag : flags) {
                std::string name = a.GetFlagName(flag);
                if (name.empty()) continue;
                out[name] = EnumToString(a.GetFlagRole(flag, 0));
            }
            return out;
        },

        "get_flag_role", sol::overload(
            [](Architecture& a, const std::string& flag_name)
                -> std::string {
                std::vector<uint32_t> flags = a.GetAllFlags();
                for (uint32_t flag : flags) {
                    if (a.GetFlagName(flag) == flag_name) {
                        return EnumToString(a.GetFlagRole(flag, 0));
                    }
                }
                return "unknown";
            },
            [](Architecture& a, const std::string& flag_name,
               uint32_t sem_class) -> std::string {
                std::vector<uint32_t> flags = a.GetAllFlags();
                for (uint32_t flag : flags) {
                    if (a.GetFlagName(flag) == flag_name) {
                        return EnumToString(a.GetFlagRole(flag, sem_class));
                    }
                }
                return "unknown";
            }
        ),

        // Decoding raw bytes

        "get_instruction_info", [](sol::this_state ts, Architecture& a,
                                    const std::string& bytes,
                                    sol::object addr_obj) -> sol::object {
            sol::state_view l(ts);
            auto addr = AsAddress(addr_obj);
            if (!addr) {
                return sol::make_object(l, sol::nil);
            }
            InstructionInfo info;
            const uint8_t* data =
                reinterpret_cast<const uint8_t*>(bytes.data());
            if (!a.GetInstructionInfo(data, *addr, bytes.size(), info)) {
                return sol::make_object(l, sol::nil);
            }
            return sol::make_object(l, InstructionInfoToTable(l, info));
        },

        "get_instruction_text", [](sol::this_state ts, Architecture& a,
                                    const std::string& bytes,
                                    sol::object addr_obj)
                                    -> std::tuple<sol::object, size_t> {
            sol::state_view l(ts);
            auto addr = AsAddress(addr_obj);
            if (!addr) {
                return std::make_tuple(sol::make_object(l, sol::nil),
                                       static_cast<size_t>(0));
            }
            size_t len = 0;
            std::vector<InstructionTextToken> tokens;
            const uint8_t* data =
                reinterpret_cast<const uint8_t*>(bytes.data());
            size_t max_len = bytes.size();
            if (!a.GetInstructionText(data, *addr, max_len, tokens)) {
                return std::make_tuple(sol::make_object(l, sol::nil),
                                       static_cast<size_t>(0));
            }
            // GetInstructionText writes the consumed length through its
            // size_t& parameter (max_len in our call), matching the
            // Python tuple return at python/architecture.py:2991-3013.
            len = max_len;
            sol::table tok_table = InstructionTextTokensToTable(l, tokens);
            return std::make_tuple(sol::make_object(l, tok_table), len);
        },

        // Cross-arch helper: returns (arch, new_addr).

        "get_associated_arch_by_address",
        [](sol::this_state ts, Architecture& a, sol::object addr_obj)
            -> std::tuple<sol::object, HexAddress> {
            sol::state_view l(ts);
            auto addr = AsAddress(addr_obj);
            if (!addr) {
                return std::make_tuple(sol::make_object(l, sol::nil),
                                       HexAddress(0));
            }
            uint64_t working = *addr;
            Ref<Architecture> result =
                a.GetAssociatedArchitectureByAddress(working);
            if (!result) {
                return std::make_tuple(sol::make_object(l, sol::nil),
                                       HexAddress(working));
            }
            return std::make_tuple(sol::make_object(l, result),
                                   HexAddress(working));
        },

        // Equality by underlying BN object pointer.

        sol::meta_function::equal_to, [](Architecture& lhs,
                                         Architecture& rhs) -> bool {
            return Architecture::GetObject(&lhs) ==
                   Architecture::GetObject(&rhs);
        },

        sol::meta_function::to_string, [](Architecture& a) -> std::string {
            return fmt::format("<Architecture: {}>", a.GetName());
        }
    );

    // Static accessors. Mirror Architecture's Python metaclass at
    // python/architecture.py:618 - get_by_name, list, contains.
    lua["Architecture"] = lua.create_table_with(
        "get_by_name", [](const std::string& name) -> Ref<Architecture> {
            return Architecture::GetByName(name);
        },
        "list", [](sol::this_state ts) -> sol::table {
            return ToLuaTable(ts, Architecture::GetList());
        },
        "contains", [](const std::string& name) -> bool {
            Ref<Architecture> arch = Architecture::GetByName(name);
            return !(!arch);
        }
    );

    if (logger) logger->LogDebug("Architecture bindings registered");
}

}  // namespace BinjaLua
