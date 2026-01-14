// Sol2 BasicBlock bindings for binja-lua

#include "common.h"

namespace BinjaLua {

// Helper to convert BNBranchType to string
static const char* BranchTypeToString(BNBranchType type) {
    switch (type) {
        case UnconditionalBranch: return "unconditional";
        case FalseBranch: return "false";
        case TrueBranch: return "true";
        case CallDestination: return "call";
        case FunctionReturn: return "return";
        case SystemCall: return "syscall";
        case IndirectBranch: return "indirect";
        case ExceptionBranch: return "exception";
        case UnresolvedBranch: return "unresolved";
        case UserDefinedBranch: return "user_defined";
        default: return "unknown";
    }
}

// Helper to create edge table
static sol::table CreateEdgeTable(sol::state_view& lua, const BasicBlockEdge& edge) {
    sol::table t = lua.create_table();
    t["type"] = BranchTypeToString(edge.type);
    t["target_addr"] = HexAddress(edge.target->GetStart());
    t["back_edge"] = edge.backEdge;
    t["fall_through"] = edge.fallThrough;
    t["target"] = edge.target;
    return t;
}

void RegisterBasicBlockBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering BasicBlock bindings");

    lua.new_usertype<BasicBlock>(BASICBLOCK_METATABLE,
        sol::no_constructor,

        // Core properties
        "start_addr", sol::property([](BasicBlock& b) -> HexAddress {
            return HexAddress(b.GetStart());
        }),

        "end_addr", sol::property([](BasicBlock& b) -> HexAddress {
            return HexAddress(b.GetEnd());
        }),

        "length", sol::property([](BasicBlock& b) -> uint64_t {
            return b.GetLength();
        }),

        "index", sol::property([](BasicBlock& b) -> size_t {
            return b.GetIndex();
        }),

        // Parent references
        "function", sol::property([](BasicBlock& b) -> Ref<Function> {
            return b.GetFunction();
        }),

        "arch", sol::property([](BasicBlock& b) -> std::string {
            Ref<Architecture> arch = b.GetArchitecture();
            return arch ? arch->GetName() : "";
        }),

        // Boolean properties
        "can_exit", sol::property([](BasicBlock& b) -> bool {
            return b.CanExit();
        }),

        "has_undetermined_outgoing_edges", sol::property([](BasicBlock& b) -> bool {
            return b.HasUndeterminedOutgoingEdges();
        }),

        "has_invalid_instructions", sol::property([](BasicBlock& b) -> bool {
            return b.HasInvalidInstructions();
        }),

        "is_il", sol::property([](BasicBlock& b) -> bool {
            return b.IsILBlock();
        }),

        "is_llil", sol::property([](BasicBlock& b) -> bool {
            return b.IsLowLevelILBlock();
        }),

        "is_mlil", sol::property([](BasicBlock& b) -> bool {
            return b.IsMediumLevelILBlock();
        }),

        // Instruction count
        "instruction_count", [](BasicBlock& b) -> size_t {
            Ref<Architecture> arch = b.GetArchitecture();
            Ref<Function> func = b.GetFunction();
            if (!func) return 0;
            Ref<BinaryView> view = func->GetView();
            if (!view || !arch) return 0;

            uint64_t addr = b.GetStart();
            uint64_t end = b.GetEnd();
            size_t count = 0;

            while (addr < end) {
                size_t instrLen = view->GetInstructionLength(arch, addr);
                if (instrLen == 0) instrLen = 1;
                count++;
                addr += instrLen;
            }
            return count;
        },

        // Edge methods
        "outgoing_edges", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto edges = b.GetOutgoingEdges();
            for (size_t i = 0; i < edges.size(); i++) {
                result[i + 1] = CreateEdgeTable(lua, edges[i]);
            }
            return result;
        },

        "incoming_edges", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto edges = b.GetIncomingEdges();
            for (size_t i = 0; i < edges.size(); i++) {
                result[i + 1] = CreateEdgeTable(lua, edges[i]);
            }
            return result;
        },

        // Dominator analysis
        "dominators", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetDominators(false);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        "strict_dominators", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetStrictDominators(false);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        "immediate_dominator", [](BasicBlock& b) -> Ref<BasicBlock> {
            return b.GetImmediateDominator(false);
        },

        "dominator_tree_children", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetDominatorTreeChildren(false);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        "dominance_frontier", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetDominanceFrontier(false);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        // Post-dominator analysis (pass true to get post-dominators)
        "post_dominators", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetDominators(true);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        "strict_post_dominators", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetStrictDominators(true);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        "immediate_post_dominator", [](BasicBlock& b) -> Ref<BasicBlock> {
            return b.GetImmediateDominator(true);
        },

        "post_dominator_tree_children", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetDominatorTreeChildren(true);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        "post_dominance_frontier", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            auto blocks = b.GetDominanceFrontier(true);
            int idx = 1;
            for (const auto& block : blocks) {
                result[idx++] = block;
            }
            return result;
        },

        // Dominance query methods
        "dominates", [](BasicBlock& b, BasicBlock& other) -> bool {
            auto dominators = other.GetDominators(false);
            for (const auto& dom : dominators) {
                if (dom->GetStart() == b.GetStart() &&
                    dom->GetEnd() == b.GetEnd()) {
                    return true;
                }
            }
            return false;
        },

        "strictly_dominates", [](BasicBlock& b, BasicBlock& other) -> bool {
            if (b.GetStart() == other.GetStart() &&
                b.GetEnd() == other.GetEnd()) {
                return false;
            }
            auto dominators = other.GetDominators(false);
            for (const auto& dom : dominators) {
                if (dom->GetStart() == b.GetStart() &&
                    dom->GetEnd() == b.GetEnd()) {
                    return true;
                }
            }
            return false;
        },

        "post_dominates", [](BasicBlock& b, BasicBlock& other) -> bool {
            auto postDoms = other.GetDominators(true);
            for (const auto& dom : postDoms) {
                if (dom->GetStart() == b.GetStart() &&
                    dom->GetEnd() == b.GetEnd()) {
                    return true;
                }
            }
            return false;
        },

        // Disassembly text (detailed with tokens)
        "disassembly_text", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            Ref<Function> func = b.GetFunction();
            if (!func) return result;

            auto lines = b.GetDisassemblyText(DisassemblySettings::GetDefaultSettings());
            for (size_t i = 0; i < lines.size(); i++) {
                sol::table line = lua.create_table();
                line["addr"] = HexAddress(lines[i].addr);

                std::string text;
                for (const auto& token : lines[i].tokens) {
                    text += token.text;
                }
                line["text"] = text;

                sol::table tokens = lua.create_table();
                for (size_t j = 0; j < lines[i].tokens.size(); j++) {
                    sol::table tok = lua.create_table();
                    tok["text"] = lines[i].tokens[j].text;
                    tok["type"] = static_cast<int>(lines[i].tokens[j].type);
                    tok["value"] = lines[i].tokens[j].value;
                    tokens[j + 1] = tok;
                }
                line["tokens"] = tokens;

                result[i + 1] = line;
            }
            return result;
        },

        // Simple disassembly (addr + text only)
        "disassembly", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            Ref<Function> func = b.GetFunction();
            if (!func) return result;

            auto lines = b.GetDisassemblyText(DisassemblySettings::GetDefaultSettings());
            for (size_t i = 0; i < lines.size(); i++) {
                sol::table line = lua.create_table();
                line["addr"] = HexAddress(lines[i].addr);

                std::string text;
                for (const auto& token : lines[i].tokens) {
                    text += token.text;
                }
                line["text"] = text;

                result[i + 1] = line;
            }
            return result;
        },

        // Instruction iteration
        "instructions", [](sol::this_state ts, BasicBlock& b) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();

            Ref<Function> func = b.GetFunction();
            Ref<Architecture> arch = b.GetArchitecture();
            if (!func || !arch) return result;

            Ref<BinaryView> view = func->GetView();
            if (!view) return result;

            uint64_t addr = b.GetStart();
            uint64_t end = b.GetEnd();
            int idx = 1;

            while (addr < end) {
                size_t len = view->GetInstructionLength(arch, addr);
                if (len == 0) len = 1;

                sol::table instr = lua.create_table();
                instr["addr"] = HexAddress(addr);
                instr["length"] = len;

                std::vector<InstructionTextToken> tokens;
                DataBuffer data = view->ReadBuffer(addr, len);
                const uint8_t* dataPtr = static_cast<const uint8_t*>(data.GetData());
                if (arch->GetInstructionText(dataPtr, addr, len, tokens)) {
                    std::string text;
                    for (const auto& tok : tokens) {
                        text += tok.text;
                    }
                    instr["text"] = text;
                }

                instr["bytes"] = std::string(
                    reinterpret_cast<const char*>(data.GetData()),
                    data.GetLength());

                result[idx++] = instr;
                addr += len;
            }
            return result;
        },

        // Comparison
        sol::meta_function::equal_to, [](BasicBlock& a, BasicBlock& b) -> bool {
            return a.GetStart() == b.GetStart() && a.GetEnd() == b.GetEnd();
        },

        sol::meta_function::less_than, [](BasicBlock& a, BasicBlock& b) -> bool {
            return a.GetStart() < b.GetStart();
        },

        // String representation
        sol::meta_function::to_string, [](BasicBlock& b) -> std::string {
            Ref<Architecture> arch = b.GetArchitecture();
            std::string archName = arch ? arch->GetName() : "unknown";
            return fmt::format("<BasicBlock: {}@0x{:x}-0x{:x} ({} bytes)>",
                             archName, b.GetStart(), b.GetEnd(), b.GetLength());
        }
    );

    if (logger) logger->LogDebug("BasicBlock bindings registered");
}

} // namespace BinjaLua
