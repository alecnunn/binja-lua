// Sol2 IL bindings for binja-lua (LLIL, MLIL, HLIL)

#include "common.h"
#include <sstream>

namespace BinjaLua {

void RegisterILBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering IL bindings");

    // LLIL bindings
    lua.new_usertype<LowLevelILFunction>(LLIL_METATABLE,
        sol::no_constructor,

        "instruction_count", sol::property([](LowLevelILFunction& il) -> size_t {
            return il.GetInstructionCount();
        }),

        "basic_block_count", sol::property([](LowLevelILFunction& il) -> size_t {
            return il.GetBasicBlocks().size();
        }),

        "get_function", [](LowLevelILFunction& il) -> Ref<Function> {
            return il.GetFunction();
        },

        "instruction_at", [](sol::this_state ts, LowLevelILFunction& il, size_t index) -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) return sol::nil;
            sol::table result = lua.create_table();
            result["index"] = index;
            return result;
        },

        "get_text", [](LowLevelILFunction& il, size_t index) -> std::string {
            if (index >= il.GetInstructionCount()) return "";
            std::vector<InstructionTextToken> tokens;
            Ref<Function> func = il.GetFunction();
            if (func && il.GetInstructionText(func, func->GetArchitecture(), index, tokens)) {
                std::ostringstream ss;
                for (const auto& token : tokens) ss << token.text;
                return ss.str();
            }
            return "LLIL instruction";
        },

        // Create flow graph from LLIL
        "create_graph", [](LowLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraph();
        },

        "create_graph_immediate", [](LowLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraphImmediate();
        },

        sol::meta_function::equal_to, [](LowLevelILFunction& a, LowLevelILFunction& b) -> bool {
            return LowLevelILFunction::GetObject(&a) == LowLevelILFunction::GetObject(&b);
        },

        sol::meta_function::to_string, [](LowLevelILFunction& il) -> std::string {
            return fmt::format("<LLIL: {} instructions>", il.GetInstructionCount());
        }
    );

    // MLIL bindings
    lua.new_usertype<MediumLevelILFunction>(MLIL_METATABLE,
        sol::no_constructor,

        "instruction_count", sol::property([](MediumLevelILFunction& il) -> size_t {
            return il.GetInstructionCount();
        }),

        "basic_block_count", sol::property([](MediumLevelILFunction& il) -> size_t {
            return il.GetBasicBlocks().size();
        }),

        "get_function", [](MediumLevelILFunction& il) -> Ref<Function> {
            return il.GetFunction();
        },

        "instruction_at", [](sol::this_state ts, MediumLevelILFunction& il, size_t index) -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) return sol::nil;
            sol::table result = lua.create_table();
            result["index"] = index;
            return result;
        },

        "get_text", [](MediumLevelILFunction& il, size_t index) -> std::string {
            if (index >= il.GetInstructionCount()) return "";
            std::vector<InstructionTextToken> tokens;
            Ref<Function> func = il.GetFunction();
            if (func && il.GetInstructionText(func, func->GetArchitecture(), index, tokens)) {
                std::ostringstream ss;
                for (const auto& token : tokens) ss << token.text;
                return ss.str();
            }
            return "MLIL instruction";
        },

        // Create flow graph from MLIL
        "create_graph", [](MediumLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraph();
        },

        "create_graph_immediate", [](MediumLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraphImmediate();
        },

        sol::meta_function::equal_to, [](MediumLevelILFunction& a, MediumLevelILFunction& b) -> bool {
            return MediumLevelILFunction::GetObject(&a) == MediumLevelILFunction::GetObject(&b);
        },

        sol::meta_function::to_string, [](MediumLevelILFunction& il) -> std::string {
            return fmt::format("<MLIL: {} instructions>", il.GetInstructionCount());
        }
    );

    // HLIL bindings
    lua.new_usertype<HighLevelILFunction>(HLIL_METATABLE,
        sol::no_constructor,

        "instruction_count", sol::property([](HighLevelILFunction& il) -> size_t {
            return il.GetInstructionCount();
        }),

        "basic_block_count", sol::property([](HighLevelILFunction& il) -> size_t {
            return il.GetBasicBlocks().size();
        }),

        "get_function", [](HighLevelILFunction& il) -> Ref<Function> {
            return il.GetFunction();
        },

        "instruction_at", [](sol::this_state ts, HighLevelILFunction& il, size_t index) -> sol::object {
            sol::state_view lua(ts);
            if (index >= il.GetInstructionCount()) return sol::nil;
            sol::table result = lua.create_table();
            result["index"] = index;
            return result;
        },

        "get_text", [](HighLevelILFunction& il, size_t index) -> std::string {
            if (index >= il.GetInstructionCount()) return "";
            auto textLines = il.GetInstructionText(index);
            if (!textLines.empty()) {
                std::ostringstream ss;
                bool first = true;
                for (const auto& line : textLines) {
                    if (!first) ss << "\n";
                    for (const auto& token : line.tokens) ss << token.text;
                    first = false;
                }
                return ss.str();
            }
            return "HLIL instruction";
        },

        // Create flow graph from HLIL
        "create_graph", [](HighLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraph();
        },

        "create_graph_immediate", [](HighLevelILFunction& il) -> Ref<FlowGraph> {
            return il.CreateFunctionGraphImmediate();
        },

        sol::meta_function::equal_to, [](HighLevelILFunction& a, HighLevelILFunction& b) -> bool {
            return HighLevelILFunction::GetObject(&a) == HighLevelILFunction::GetObject(&b);
        },

        sol::meta_function::to_string, [](HighLevelILFunction& il) -> std::string {
            return fmt::format("<HLIL: {} instructions>", il.GetInstructionCount());
        }
    );

    if (logger) logger->LogDebug("IL bindings registered");
}

} // namespace BinjaLua
