// Sol2 Platform bindings for binja-lua
// Read-only binding for BinaryNinja::Platform (R6). Mirrors the Python
// CorePlatform read surface at python/platform.py:797.
//
// Deferred to R12 (type library wave) or beyond:
//   - type_libraries / get_type_libraries_by_name - needs TypeLibrary
//     usertype
//   - type_container (python/platform.py:629) - needs TypeContainer
//     usertype
//   - parse_types_from_source / parse_types_from_source_file - returns
//     three maps plus an error string, better landed with the
//     TypeParser work
// Deferred indefinitely (write-side / plugin authoring):
//   - Register / RegisterCallingConvention / RegisterDefaultCalling... /
//     RegisterCdecl... / RegisterStdcall... / RegisterFastcall... /
//     SetSystemCallConvention / BinaryViewInit / AdjustTypeParserInput
//   - AddRelatedPlatform / GenerateAutoPlatformType*

#include "common.h"

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace BinjaLua {

namespace {

// Build a Lua table keyed by QualifiedName string, mirroring the
// Python Platform.types / variables / functions accessors at
// python/platform.py:507-533.
sol::table QualifiedTypeMapToTable(
    sol::state_view lua,
    const std::map<QualifiedName, Ref<Type>>& entries) {
    sol::table result =
        lua.create_table(0, static_cast<int>(entries.size()));
    for (const auto& kv : entries) {
        result[kv.first.GetString()] = kv.second;
    }
    return result;
}

}  // namespace

void RegisterPlatformBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Platform bindings");

    lua.new_usertype<Platform>(PLATFORM_METATABLE,
        sol::no_constructor,

        // Identity

        "name", sol::property([](Platform& p) -> std::string {
            return p.GetName();
        }),

        "arch", sol::property([](Platform& p) -> Ref<Architecture> {
            return p.GetArchitecture();
        }),

        "address_size", sol::property([](Platform& p) -> size_t {
            return p.GetAddressSize();
        }),

        // Calling conventions. Every accessor returns a single
        // Ref<CallingConvention> or nil, matching the null-passthrough
        // shape at python/platform.py:392-493.

        "default_calling_convention",
        sol::property([](Platform& p) -> Ref<CallingConvention> {
            return p.GetDefaultCallingConvention();
        }),

        "cdecl_calling_convention",
        sol::property([](Platform& p) -> Ref<CallingConvention> {
            return p.GetCdeclCallingConvention();
        }),

        "stdcall_calling_convention",
        sol::property([](Platform& p) -> Ref<CallingConvention> {
            return p.GetStdcallCallingConvention();
        }),

        "fastcall_calling_convention",
        sol::property([](Platform& p) -> Ref<CallingConvention> {
            return p.GetFastcallCallingConvention();
        }),

        "system_call_convention",
        sol::property([](Platform& p) -> Ref<CallingConvention> {
            return p.GetSystemCallConvention();
        }),

        "calling_conventions",
        [](sol::this_state ts, Platform& p) -> sol::table {
            return ToLuaTable(ts, p.GetCallingConventions());
        },

        // Global registers

        "global_regs", sol::property([](sol::this_state ts, Platform& p)
                                          -> sol::table {
            sol::state_view l(ts);
            std::vector<uint32_t> regs = p.GetGlobalRegisters();
            Ref<Architecture> arch = p.GetArchitecture();
            sol::table out = l.create_table(static_cast<int>(regs.size()), 0);
            int idx = 1;
            for (uint32_t reg : regs) {
                if (!arch) continue;
                std::string name = arch->GetRegisterName(reg);
                if (name.empty()) continue;
                out[idx++] = name;
            }
            return out;
        }),

        "get_global_register_type", [](Platform& p, uint32_t reg)
                                         -> Ref<Type> {
            return p.GetGlobalRegisterType(reg);
        },

        // Related platforms

        "get_related_platform", [](Platform& p, Architecture& arch)
                                     -> Ref<Platform> {
            return p.GetRelatedPlatform(&arch);
        },

        "related_platforms", [](sol::this_state ts, Platform& p)
                                  -> sol::table {
            return ToLuaTable(ts, p.GetRelatedPlatforms());
        },

        "get_associated_platform_by_address",
        [](sol::this_state ts, Platform& p, sol::object addr_obj)
            -> std::tuple<sol::object, HexAddress> {
            sol::state_view l(ts);
            auto addr = AsAddress(addr_obj);
            if (!addr) {
                return std::make_tuple(sol::make_object(l, sol::nil),
                                       HexAddress(0));
            }
            uint64_t working = *addr;
            Ref<Platform> result = p.GetAssociatedPlatformByAddress(working);
            if (!result) {
                return std::make_tuple(sol::make_object(l, sol::nil),
                                       HexAddress(working));
            }
            return std::make_tuple(sol::make_object(l, result),
                                   HexAddress(working));
        },

        // Platform types. The four accessors here match
        // python/platform.py:507-546. type_libraries and type_container
        // stay deferred (see the R12 block in docs/extension-plan.md).

        "types", sol::property([](sol::this_state ts, Platform& p)
                                    -> sol::table {
            return QualifiedTypeMapToTable(sol::state_view(ts), p.GetTypes());
        }),

        "variables", sol::property([](sol::this_state ts, Platform& p)
                                        -> sol::table {
            return QualifiedTypeMapToTable(sol::state_view(ts),
                                           p.GetVariables());
        }),

        "functions", sol::property([](sol::this_state ts, Platform& p)
                                        -> sol::table {
            return QualifiedTypeMapToTable(sol::state_view(ts),
                                           p.GetFunctions());
        }),

        "system_calls", sol::property([](sol::this_state ts, Platform& p)
                                           -> sol::table {
            sol::state_view l(ts);
            std::map<uint32_t, QualifiedNameAndType> calls =
                p.GetSystemCalls();
            sol::table out = l.create_table(0, static_cast<int>(calls.size()));
            for (const auto& kv : calls) {
                sol::table entry = l.create_table(0, 2);
                entry["name"] = kv.second.name.GetString();
                entry["type"] = kv.second.type;
                out[kv.first] = entry;
            }
            return out;
        }),

        "get_type_by_name", [](Platform& p, const std::string& name)
                                 -> Ref<Type> {
            return p.GetTypeByName(QualifiedName(name));
        },

        "get_variable_by_name", [](Platform& p, const std::string& name)
                                     -> Ref<Type> {
            return p.GetVariableByName(QualifiedName(name));
        },

        "get_function_by_name", sol::overload(
            [](Platform& p, const std::string& name) -> Ref<Type> {
                return p.GetFunctionByName(QualifiedName(name), false);
            },
            [](Platform& p, const std::string& name, bool exact_match)
                -> Ref<Type> {
                return p.GetFunctionByName(QualifiedName(name), exact_match);
            }
        ),

        "get_system_call_name", [](Platform& p, uint32_t n) -> std::string {
            return p.GetSystemCallName(n);
        },

        "get_system_call_type", [](Platform& p, uint32_t n) -> Ref<Type> {
            return p.GetSystemCallType(n);
        },

        sol::meta_function::equal_to, [](Platform& lhs, Platform& rhs)
                                           -> bool {
            return Platform::GetObject(&lhs) == Platform::GetObject(&rhs);
        },

        sol::meta_function::to_string, [](Platform& p) -> std::string {
            Ref<Architecture> arch = p.GetArchitecture();
            return fmt::format("<Platform: {} ({})>",
                               p.GetName(),
                               arch ? arch->GetName() : "<unknown>");
        }
    );

    // Static accessors. Python exposes get_list / list_by_arch /
    // list_by_os as the four GetList overloads plus GetOSList at
    // python/platform.py:40-390.
    lua["Platform"] = lua.create_table_with(
        "get_by_name", [](const std::string& name) -> Ref<Platform> {
            return Platform::GetByName(name);
        },
        "list", [](sol::this_state ts) -> sol::table {
            return ToLuaTable(ts, Platform::GetList());
        },
        "list_by_arch", [](sol::this_state ts, Architecture& arch)
                             -> sol::table {
            return ToLuaTable(ts, Platform::GetList(&arch));
        },
        "list_by_os", [](sol::this_state ts, const std::string& os)
                           -> sol::table {
            return ToLuaTable(ts, Platform::GetList(os));
        },
        "os_list", [](sol::this_state ts) -> sol::table {
            return ToLuaTable(ts, Platform::GetOSList());
        }
    );

    if (logger) logger->LogDebug("Platform bindings registered");
}

}  // namespace BinjaLua
