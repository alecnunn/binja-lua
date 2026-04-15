// Sol2 Selection bindings for binja-lua

#include "common.h"

namespace BinjaLua {

void RegisterSelectionBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering Selection bindings");

    lua.new_usertype<Selection>(SELECTION_METATABLE,
        sol::constructors<Selection(), Selection(uint64_t, uint64_t)>(),

        // Properties
        "start_addr", sol::property(
            [](const Selection& s) { return HexAddress(s.start); },
            [](Selection& s, uint64_t v) { s.start = v; }
        ),
        "end_addr", sol::property(
            [](const Selection& s) { return HexAddress(s.end); },
            [](Selection& s, uint64_t v) { s.end = v; }
        ),

        // Methods
        "length", &Selection::length,

        // String representation
        sol::meta_function::to_string, [](const Selection& s) {
            return fmt::format("Selection(0x{:x}-0x{:x}, {} bytes)",
                             s.start, s.end, s.length());
        }
    );

    if (logger) logger->LogDebug("Selection bindings registered");
}

}  // namespace BinjaLua
