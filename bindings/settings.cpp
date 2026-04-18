// Sol2 Settings bindings for binja-lua (R8).
// Mirrors the Python binaryninja.settings.Settings surface at
// python/settings.py:31 but keeps the typed getter/setter split rather
// than collapsing onto a single Get<T>. Each scalar kind (bool, int64,
// uint64, double, string, string list, json) has its own method so Lua
// callers never need to specify a template argument.
//
// sol2 3.3.0 HARD RULE: NEVER combine sol::property with sol::this_state
// in a lambda parameter list - it compiles and loads fine and then
// crashes BN on first access under MSVC. Every accessor that needs the
// Lua state to build a table (keys, get_string_list, the *_with_scope
// methods) is bound as a plain method so Lua calls it with colon
// syntax (`s:keys()`). Confirmed via task #12 / #14 / #15 / #16.
// Before committing any edit to this file, grep for
// `sol::property.*sol::this_state` and verify no matches.

#include "common.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace BinjaLua {

namespace {

// A Settings getter/setter can operate on a BinaryView scope, a
// Function scope, or the default/user/project store (neither). Python
// collapses this onto an Optional[Union[BinaryView, Function]]; we
// mirror the shape by letting the `resource` argument be nil, a
// BinaryView usertype, or a Function usertype. Unrecognised types
// collapse to "no resource" rather than raising, matching how the
// existing bindings treat address coercion in AsAddress.
struct ResourceHandles {
    Ref<BinaryView> view;
    Ref<Function> func;
};

ResourceHandles AsResource(sol::object obj) {
    ResourceHandles handles;
    if (!obj.valid() || obj.get_type() == sol::type::nil) {
        return handles;
    }
    if (obj.is<Ref<BinaryView>>()) {
        handles.view = obj.as<Ref<BinaryView>>();
    } else if (obj.is<Ref<Function>>()) {
        handles.func = obj.as<Ref<Function>>();
    }
    return handles;
}

// Resolve the optional scope argument. Accepts nil (defaults to
// SettingsAutoScope) or a string routed through the R2 dual-accept
// EnumFromString<BNSettingsScope> helper. An unrecognised string also
// collapses to SettingsAutoScope so a misspelled scope does not break
// the call chain - the UpdateProperty / serialize paths already
// tolerate SettingsAutoScope.
BNSettingsScope AsScope(sol::object obj) {
    if (!obj.valid() || obj.get_type() == sol::type::nil) {
        return SettingsAutoScope;
    }
    if (obj.is<std::string>()) {
        auto parsed = EnumFromString<BNSettingsScope>(obj.as<std::string>());
        if (parsed) return *parsed;
    }
    return SettingsAutoScope;
}

// Accept Lua integer / HexAddress / double for uint64 setting values.
// Lua 5.4 integers are 64-bit signed internally; letting sol2 coerce a
// large-magnitude unsigned key to int64 and then re-casting preserves
// the bit pattern, which is what BN's unsigned-integer settings want.
uint64_t AsUInt64(sol::object obj) {
    if (obj.is<uint64_t>()) return obj.as<uint64_t>();
    if (obj.is<int64_t>())
        return static_cast<uint64_t>(obj.as<int64_t>());
    if (obj.is<double>()) return static_cast<uint64_t>(obj.as<double>());
    return 0;
}

std::vector<std::string> TableAsStringList(sol::table tbl) {
    std::vector<std::string> out;
    size_t n = tbl.size();
    out.reserve(n);
    for (size_t i = 1; i <= n; ++i) {
        sol::object v = tbl[i];
        if (v.is<std::string>()) {
            out.push_back(v.as<std::string>());
        }
    }
    return out;
}

}  // namespace

void RegisterSettingsBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Settings bindings");

    lua.new_usertype<Settings>(SETTINGS_METATABLE,
        sol::no_constructor,

        // Identity. The instance_id is only tracked on the C++ side
        // when the Settings object was constructed via
        // Settings::Instance; instances handed in from the core API
        // may report empty. Callers that need a stable identifier
        // should fall back to tostring(s).

        // Schema introspection. Every accessor that takes a resource
        // argument routes it through AsResource so a BinaryView, a
        // Function, or nil all work uniformly.

        "contains", [](Settings& s, const std::string& key) -> bool {
            return s.Contains(key);
        },

        "keys", [](sol::this_state ts, Settings& s) -> sol::table {
            return ToLuaTable(ts, s.Keys(),
                              [](const std::string& v) -> std::string {
                                  return v;
                              });
        },

        "is_empty",
        [](Settings& s, sol::object resource_obj, sol::object scope_obj)
            -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.IsEmpty(r.view, scope);
            if (r.func) return s.IsEmpty(r.func, scope);
            return s.IsEmpty(scope);
        },

        "query_property_string",
        [](Settings& s, const std::string& key,
           const std::string& property) -> std::string {
            return s.QueryProperty<std::string>(key, property);
        },

        "query_property_string_list",
        [](sol::this_state ts, Settings& s, const std::string& key,
           const std::string& property) -> sol::table {
            std::vector<std::string> values =
                s.QueryProperty<std::vector<std::string>>(key, property);
            return ToLuaTable(ts, values,
                              [](const std::string& v) -> std::string {
                                  return v;
                              });
        },

        // Schema mutation.

        "register_group",
        [](Settings& s, const std::string& group,
           const std::string& title) -> bool {
            return s.RegisterGroup(group, title);
        },

        "register_setting",
        [](Settings& s, const std::string& key,
           const std::string& properties) -> bool {
            return s.RegisterSetting(key, properties);
        },

        "update_property",
        [](Settings& s, const std::string& key,
           const std::string& property) -> bool {
            return s.UpdateProperty(key, property);
        },

        "deserialize_schema",
        [](Settings& s, const std::string& schema, sol::object scope_obj,
           sol::object merge_obj) -> bool {
            BNSettingsScope scope = AsScope(scope_obj);
            bool merge = merge_obj.is<bool>() ? merge_obj.as<bool>()
                                              : true;
            return s.DeserializeSchema(schema, scope, merge);
        },

        "serialize_schema", [](Settings& s) -> std::string {
            return s.SerializeSchema();
        },

        // Typed getters. The Python surface exposes get_bool /
        // get_integer / get_double / get_string / get_string_list /
        // get_json; we keep that split but use uint64 for
        // get_integer because BN's "integer" settings are unsigned
        // (matching python/settings.py:380-390).

        "get_bool",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> bool {
            ResourceHandles r = AsResource(resource_obj);
            if (r.view) return s.Get<bool>(key, r.view, nullptr);
            if (r.func) return s.Get<bool>(key, r.func, nullptr);
            return s.Get<bool>(key, Ref<BinaryView>(nullptr), nullptr);
        },

        "get_double",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> double {
            ResourceHandles r = AsResource(resource_obj);
            if (r.view) return s.Get<double>(key, r.view, nullptr);
            if (r.func) return s.Get<double>(key, r.func, nullptr);
            return s.Get<double>(key, Ref<BinaryView>(nullptr), nullptr);
        },

        "get_integer",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> uint64_t {
            ResourceHandles r = AsResource(resource_obj);
            if (r.view) return s.Get<uint64_t>(key, r.view, nullptr);
            if (r.func) return s.Get<uint64_t>(key, r.func, nullptr);
            return s.Get<uint64_t>(key, Ref<BinaryView>(nullptr), nullptr);
        },

        "get_string",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> std::string {
            ResourceHandles r = AsResource(resource_obj);
            if (r.view) return s.Get<std::string>(key, r.view, nullptr);
            if (r.func) return s.Get<std::string>(key, r.func, nullptr);
            return s.Get<std::string>(key, Ref<BinaryView>(nullptr),
                                      nullptr);
        },

        "get_string_list",
        [](sol::this_state ts, Settings& s, const std::string& key,
           sol::object resource_obj) -> sol::table {
            ResourceHandles r = AsResource(resource_obj);
            std::vector<std::string> values;
            if (r.view) {
                values = s.Get<std::vector<std::string>>(key, r.view,
                                                         nullptr);
            } else if (r.func) {
                values = s.Get<std::vector<std::string>>(key, r.func,
                                                         nullptr);
            } else {
                values = s.Get<std::vector<std::string>>(
                    key, Ref<BinaryView>(nullptr), nullptr);
            }
            return ToLuaTable(ts, values,
                              [](const std::string& v) -> std::string {
                                  return v;
                              });
        },

        "get_json",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> std::string {
            ResourceHandles r = AsResource(resource_obj);
            if (r.view) return s.GetJson(key, r.view, nullptr);
            if (r.func) return s.GetJson(key, r.func, nullptr);
            return s.GetJson(key, Ref<BinaryView>(nullptr), nullptr);
        },

        // Scope-aware getters. Return (value, scope_string) matching
        // python/settings.py:435-524. The scope string is the short
        // canonical form from EnumToString, so scripts can branch on
        // "default"/"user"/"project"/"resource" without importing
        // integer constants.

        "get_bool_with_scope",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> std::tuple<bool, std::string> {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = SettingsAutoScope;
            bool value;
            if (r.view) {
                value = s.Get<bool>(key, r.view, &scope);
            } else if (r.func) {
                value = s.Get<bool>(key, r.func, &scope);
            } else {
                value = s.Get<bool>(key, Ref<BinaryView>(nullptr), &scope);
            }
            return std::make_tuple(value, std::string(EnumToString(scope)));
        },

        "get_double_with_scope",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> std::tuple<double, std::string> {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = SettingsAutoScope;
            double value;
            if (r.view) {
                value = s.Get<double>(key, r.view, &scope);
            } else if (r.func) {
                value = s.Get<double>(key, r.func, &scope);
            } else {
                value = s.Get<double>(key, Ref<BinaryView>(nullptr),
                                      &scope);
            }
            return std::make_tuple(value, std::string(EnumToString(scope)));
        },

        "get_integer_with_scope",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> std::tuple<uint64_t, std::string> {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = SettingsAutoScope;
            uint64_t value;
            if (r.view) {
                value = s.Get<uint64_t>(key, r.view, &scope);
            } else if (r.func) {
                value = s.Get<uint64_t>(key, r.func, &scope);
            } else {
                value = s.Get<uint64_t>(key, Ref<BinaryView>(nullptr),
                                        &scope);
            }
            return std::make_tuple(value, std::string(EnumToString(scope)));
        },

        "get_string_with_scope",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> std::tuple<std::string, std::string> {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = SettingsAutoScope;
            std::string value;
            if (r.view) {
                value = s.Get<std::string>(key, r.view, &scope);
            } else if (r.func) {
                value = s.Get<std::string>(key, r.func, &scope);
            } else {
                value = s.Get<std::string>(key, Ref<BinaryView>(nullptr),
                                            &scope);
            }
            return std::make_tuple(value, std::string(EnumToString(scope)));
        },

        "get_string_list_with_scope",
        [](sol::this_state ts, Settings& s, const std::string& key,
           sol::object resource_obj)
            -> std::tuple<sol::table, std::string> {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = SettingsAutoScope;
            std::vector<std::string> values;
            if (r.view) {
                values = s.Get<std::vector<std::string>>(key, r.view,
                                                         &scope);
            } else if (r.func) {
                values = s.Get<std::vector<std::string>>(key, r.func,
                                                         &scope);
            } else {
                values = s.Get<std::vector<std::string>>(
                    key, Ref<BinaryView>(nullptr), &scope);
            }
            sol::table tbl = ToLuaTable(
                ts, values,
                [](const std::string& v) -> std::string { return v; });
            return std::make_tuple(tbl, std::string(EnumToString(scope)));
        },

        "get_json_with_scope",
        [](Settings& s, const std::string& key, sol::object resource_obj)
            -> std::tuple<std::string, std::string> {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = SettingsAutoScope;
            std::string value;
            if (r.view) {
                value = s.GetJson(key, r.view, &scope);
            } else if (r.func) {
                value = s.GetJson(key, r.func, &scope);
            } else {
                value = s.GetJson(key, Ref<BinaryView>(nullptr), &scope);
            }
            return std::make_tuple(value, std::string(EnumToString(scope)));
        },

        // Typed setters. All setters take (key, value, resource?,
        // scope?), matching the Python positional ordering. A missing
        // resource maps to default/user scope via SettingsAutoScope.

        "set_bool",
        [](Settings& s, const std::string& key, bool value,
           sol::object resource_obj, sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.Set(key, value, r.view, scope);
            if (r.func) return s.Set(key, value, r.func, scope);
            return s.Set(key, value, Ref<BinaryView>(nullptr), scope);
        },

        "set_double",
        [](Settings& s, const std::string& key, double value,
           sol::object resource_obj, sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.Set(key, value, r.view, scope);
            if (r.func) return s.Set(key, value, r.func, scope);
            return s.Set(key, value, Ref<BinaryView>(nullptr), scope);
        },

        "set_integer",
        [](Settings& s, const std::string& key, sol::object value_obj,
           sol::object resource_obj, sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            uint64_t value = AsUInt64(value_obj);
            if (r.view) return s.Set(key, value, r.view, scope);
            if (r.func) return s.Set(key, value, r.func, scope);
            return s.Set(key, value, Ref<BinaryView>(nullptr), scope);
        },

        "set_string",
        [](Settings& s, const std::string& key, const std::string& value,
           sol::object resource_obj, sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.Set(key, value, r.view, scope);
            if (r.func) return s.Set(key, value, r.func, scope);
            return s.Set(key, value, Ref<BinaryView>(nullptr), scope);
        },

        "set_string_list",
        [](Settings& s, const std::string& key, sol::table value_tbl,
           sol::object resource_obj, sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            std::vector<std::string> values = TableAsStringList(value_tbl);
            if (r.view) return s.Set(key, values, r.view, scope);
            if (r.func) return s.Set(key, values, r.func, scope);
            return s.Set(key, values, Ref<BinaryView>(nullptr), scope);
        },

        "set_json",
        [](Settings& s, const std::string& key, const std::string& value,
           sol::object resource_obj, sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.SetJson(key, value, r.view, scope);
            if (r.func) return s.SetJson(key, value, r.func, scope);
            return s.SetJson(key, value, Ref<BinaryView>(nullptr), scope);
        },

        // Reset / serialization. These accept an optional resource and
        // scope; serialize_settings returns the string payload so Lua
        // scripts can round-trip settings state for tests.

        "reset",
        [](Settings& s, const std::string& key, sol::object resource_obj,
           sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.Reset(key, r.view, scope);
            if (r.func) return s.Reset(key, r.func, scope);
            return s.Reset(key, Ref<BinaryView>(nullptr), scope);
        },

        "reset_all",
        [](Settings& s, sol::object resource_obj, sol::object scope_obj,
           sol::object schema_only_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            bool schema_only = schema_only_obj.is<bool>()
                                 ? schema_only_obj.as<bool>()
                                 : true;
            if (r.view) return s.ResetAll(r.view, scope, schema_only);
            if (r.func) return s.ResetAll(r.func, scope, schema_only);
            return s.ResetAll(Ref<BinaryView>(nullptr), scope, schema_only);
        },

        "serialize_settings",
        [](Settings& s, sol::object resource_obj, sol::object scope_obj)
            -> std::string {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.SerializeSettings(r.view, scope);
            if (r.func) return s.SerializeSettings(r.func, scope);
            return s.SerializeSettings(Ref<BinaryView>(nullptr), scope);
        },

        "deserialize_settings",
        [](Settings& s, const std::string& contents,
           sol::object resource_obj, sol::object scope_obj) -> bool {
            ResourceHandles r = AsResource(resource_obj);
            BNSettingsScope scope = AsScope(scope_obj);
            if (r.view) return s.DeserializeSettings(contents, r.view, scope);
            if (r.func) return s.DeserializeSettings(contents, r.func, scope);
            return s.DeserializeSettings(contents, Ref<BinaryView>(nullptr),
                                         scope);
        },

        "load_settings_file",
        [](Settings& s, const std::string& filename, sol::object scope_obj,
           sol::object view_obj) -> bool {
            BNSettingsScope scope = AsScope(scope_obj);
            Ref<BinaryView> view;
            if (view_obj.is<Ref<BinaryView>>()) {
                view = view_obj.as<Ref<BinaryView>>();
            }
            return s.LoadSettingsFile(filename, scope, view);
        },

        "set_resource_id",
        [](Settings& s, const std::string& resource_id) {
            s.SetResourceId(resource_id);
        },

        // Metamethods.

        sol::meta_function::equal_to, [](Settings& a, Settings& b) -> bool {
            return Settings::GetObject(&a) == Settings::GetObject(&b);
        },

        sol::meta_function::to_string, [](Settings& s) -> std::string {
            return fmt::format("<Settings: {} keys>", s.Keys().size());
        }
    );

    // Static factory table. Mirrors python/settings.py:130-144 where
    // Settings() produces the "default" schema and Settings(id)
    // produces a named schema.
    lua["Settings"] = lua.create_table_with(
        "new", sol::overload(
            []() -> Ref<Settings> {
                return Settings::Instance("");
            },
            [](const std::string& schema_id) -> Ref<Settings> {
                return Settings::Instance(schema_id);
            }
        )
    );

    if (logger) logger->LogDebug("Settings bindings registered");
}

}  // namespace BinjaLua
