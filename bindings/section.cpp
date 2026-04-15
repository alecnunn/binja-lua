// Sol2 Section bindings for binja-lua

#include "common.h"

namespace BinjaLua {

void RegisterSectionBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Section bindings");

    // Register Section type - sol2 will handle Ref<Section> via unique_usertype_traits
    lua.new_usertype<Section>(SECTION_METATABLE,
        sol::no_constructor,

        // Identity
        "name", sol::property([](Section& s) -> std::string {
            return s.GetName();
        }),

        // Format-specific type tag (e.g. "PROGBITS"). Matches Python's
        // Section.type, not the semantics enum.
        "type", sol::property([](Section& s) -> std::string {
            return s.GetType();
        }),

        // Semantics enum as a stable string (default, code, ro_data,
        // rw_data, external). Replaces the old permissions triple.
        "semantics", sol::property([](Section& s) -> std::string {
            return EnumToString(s.GetSemantics());
        }),

        // Address range
        "start_addr", sol::property([](Section& s) -> HexAddress {
            return HexAddress(s.GetStart());
        }),

        "end_addr", sol::property([](Section& s) -> HexAddress {
            return HexAddress(s.GetEnd());
        }),

        "length", sol::property([](Section& s) -> uint64_t {
            return s.GetLength();
        }),

        // Layout metadata
        "align", sol::property([](Section& s) -> uint64_t {
            return s.GetAlignment();
        }),

        "entry_size", sol::property([](Section& s) -> uint64_t {
            return s.GetEntrySize();
        }),

        // Cross-section links
        "linked_section", sol::property([](Section& s) -> std::string {
            return s.GetLinkedSection();
        }),

        "info_section", sol::property([](Section& s) -> std::string {
            return s.GetInfoSection();
        }),

        "info_data", sol::property([](Section& s) -> uint64_t {
            return s.GetInfoData();
        }),

        // Whether Binary Ninja auto-detected this section
        "auto_defined", sol::property([](Section& s) -> bool {
            return s.AutoDefined();
        }),

        // Comparison operator - must provide to prevent auto-enrollment
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
