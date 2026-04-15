// Sol2 Section bindings for binja-lua

#include "common.h"

namespace BinjaLua {

void RegisterSectionBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Section bindings");

    // Register Section type - sol2 will handle Ref<Section> via unique_usertype_traits
    lua.new_usertype<Section>(SECTION_METATABLE,
        sol::no_constructor,

        // Properties - sol2 gives us Section& after dereferencing Ref<Section>
        "name", sol::property([](Section& s) -> std::string {
            return s.GetName();
        }),

        "start_addr", sol::property([](Section& s) -> HexAddress {
            return HexAddress(s.GetStart());
        }),

        "length", sol::property([](Section& s) -> uint64_t {
            return s.GetLength();
        }),

        // Methods
        "permissions", [](sol::this_state ts, Section& s) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            BNSectionSemantics semantics = s.GetSemantics();
            bool isCode = (semantics == ReadOnlyCodeSectionSemantics);
            bool isWritable = (semantics == ReadWriteDataSectionSemantics);
            result["read"] = true;
            result["write"] = isWritable;
            result["execute"] = isCode;
            return result;
        },

        "type", sol::property([](Section& s) -> std::string {
            BNSectionSemantics semantics = s.GetSemantics();
            switch (semantics) {
                case DefaultSectionSemantics: return "default";
                case ReadOnlyCodeSectionSemantics: return "code";
                case ReadOnlyDataSectionSemantics: return "data";
                case ReadWriteDataSectionSemantics: return "data";
                case ExternalSectionSemantics: return "external";
                default: return "unknown";
            }
        }),

        // Comparison operators - must provide to prevent auto-enrollment
        sol::meta_function::equal_to, [](Section& a, Section& b) -> bool {
            return Section::GetObject(&a) == Section::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](Section& s) -> std::string {
            return fmt::format("Section({}, 0x{:x}, {} bytes)",
                             s.GetName(), s.GetStart(), s.GetLength());
        }
    );

    if (logger) logger->LogDebug("Section bindings registered");
}

}  // namespace BinjaLua
