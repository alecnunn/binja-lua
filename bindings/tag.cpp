// Sol2 Tag and TagType bindings for binja-lua

#include "common.h"

namespace BinjaLua {

void RegisterTagBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Tag bindings");

    // TagType usertype
    lua.new_usertype<TagType>("BinaryNinja.TagType",
        sol::no_constructor,

        // Properties
        "id", sol::property([](TagType& tt) -> std::string {
            return tt.GetId();
        }),

        "name", sol::property([](TagType& tt) -> std::string {
            return tt.GetName();
        }),

        "icon", sol::property([](TagType& tt) -> std::string {
            return tt.GetIcon();
        }),

        "visible", sol::property([](TagType& tt) -> bool {
            return tt.GetVisible();
        }),

        "type", sol::property([](TagType& tt) -> std::string {
            switch (tt.GetType()) {
                case UserTagType: return "user";
                case NotificationTagType: return "notification";
                case BookmarksTagType: return "bookmarks";
                default: return "unknown";
            }
        }),

        // Setters
        "set_name", [](TagType& tt, const std::string& name) {
            tt.SetName(name);
        },

        "set_icon", [](TagType& tt, const std::string& icon) {
            tt.SetIcon(icon);
        },

        "set_visible", [](TagType& tt, bool visible) {
            tt.SetVisible(visible);
        },

        // Comparison
        sol::meta_function::equal_to, [](TagType& a, TagType& b) -> bool {
            return a.GetId() == b.GetId();
        },

        // String representation
        sol::meta_function::to_string, [](TagType& tt) -> std::string {
            return fmt::format("<TagType: '{}' ({})>", tt.GetName(), tt.GetIcon());
        }
    );

    // Tag usertype
    lua.new_usertype<Tag>("BinaryNinja.Tag",
        sol::no_constructor,

        // Properties
        "id", sol::property([](Tag& t) -> std::string {
            return t.GetId();
        }),

        "type", sol::property([](Tag& t) -> Ref<TagType> {
            return t.GetType();
        }),

        "data", sol::property([](Tag& t) -> std::string {
            return t.GetData();
        }),

        // Setter for data
        "set_data", [](Tag& t, const std::string& data) {
            t.SetData(data);
        },

        // Comparison
        sol::meta_function::equal_to, [](Tag& a, Tag& b) -> bool {
            return a.GetId() == b.GetId();
        },

        // String representation
        sol::meta_function::to_string, [](Tag& t) -> std::string {
            Ref<TagType> tt = t.GetType();
            std::string typeName = tt ? tt->GetName() : "unknown";
            std::string data = t.GetData();
            if (data.empty()) {
                return fmt::format("<Tag: '{}'>", typeName);
            }
            return fmt::format("<Tag: '{}' = '{}'>", typeName, data);
        }
    );

    if (logger) logger->LogDebug("Tag bindings registered");
}

} // namespace BinjaLua
