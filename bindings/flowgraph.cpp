// Sol2 FlowGraph bindings for binja-lua

#include "common.h"

namespace BinjaLua {

// Helper to convert branch type string to enum
static BNBranchType StringToBranchType(const std::string& type) {
    if (type == "unconditional") return UnconditionalBranch;
    if (type == "false") return FalseBranch;
    if (type == "true") return TrueBranch;
    if (type == "call") return CallDestination;
    if (type == "return") return FunctionReturn;
    if (type == "syscall") return SystemCall;
    if (type == "indirect") return IndirectBranch;
    if (type == "exception") return ExceptionBranch;
    return UnconditionalBranch;
}

// Helper to convert branch type enum to string
static std::string BranchTypeToString(BNBranchType type) {
    switch (type) {
        case UnconditionalBranch: return "unconditional";
        case FalseBranch: return "false";
        case TrueBranch: return "true";
        case CallDestination: return "call";
        case FunctionReturn: return "return";
        case SystemCall: return "syscall";
        case IndirectBranch: return "indirect";
        case ExceptionBranch: return "exception";
        default: return "unknown";
    }
}

void RegisterFlowGraphBindings(sol::state_view lua, Ref<Logger> logger) {
    if (logger) logger->LogDebug("Registering FlowGraph bindings");

    // FlowGraphNode usertype
    lua.new_usertype<FlowGraphNode>(FLOWGRAPHNODE_METATABLE,
        sol::no_constructor,

        // Position properties
        "x", sol::property(&FlowGraphNode::GetX, &FlowGraphNode::SetX),
        "y", sol::property(&FlowGraphNode::GetY, &FlowGraphNode::SetY),
        "width", sol::property(&FlowGraphNode::GetWidth),
        "height", sol::property(&FlowGraphNode::GetHeight),

        // Associated basic block
        "basic_block", sol::property(
            [](FlowGraphNode& node) -> Ref<BasicBlock> {
                return node.GetBasicBlock();
            },
            [](FlowGraphNode& node, Ref<BasicBlock> block) {
                node.SetBasicBlock(block);
            }
        ),

        // Get highlight color
        "highlight", sol::property([](sol::this_state ts, FlowGraphNode& node) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            BNHighlightColor color = node.GetHighlight();
            result["style"] = static_cast<int>(color.style);
            result["color"] = static_cast<int>(color.color);
            result["r"] = color.r;
            result["g"] = color.g;
            result["b"] = color.b;
            result["alpha"] = color.alpha;
            return result;
        }),

        // Get display lines
        "lines", [](sol::this_state ts, FlowGraphNode& node) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            const std::vector<DisassemblyTextLine>& lines = node.GetLines();
            for (size_t i = 0; i < lines.size(); i++) {
                sol::table lineTable = lua.create_table();
                std::string text;
                for (const auto& token : lines[i].tokens) {
                    text += token.text;
                }
                lineTable["text"] = text;
                lineTable["address"] = HexAddress(lines[i].addr);
                result[i + 1] = lineTable;
            }
            return result;
        },

        // Set display lines - accepts array of strings or tables with text field
        "set_lines", [](FlowGraphNode& node, sol::table lines) {
            std::vector<DisassemblyTextLine> lineVec;
            for (auto& kv : lines) {
                DisassemblyTextLine line;
                std::string text;

                if (kv.second.is<std::string>()) {
                    // Simple string
                    text = kv.second.as<std::string>();
                } else if (kv.second.is<sol::table>()) {
                    // Table with text and optional address
                    sol::table lineTable = kv.second.as<sol::table>();
                    text = lineTable.get_or<std::string>("text", "");

                    if (lineTable["address"].valid()) {
                        if (lineTable["address"].is<HexAddress>()) {
                            line.addr = lineTable["address"].get<HexAddress>().value;
                        } else if (lineTable["address"].is<uint64_t>()) {
                            line.addr = lineTable["address"].get<uint64_t>();
                        }
                    }
                }

                if (!text.empty()) {
                    InstructionTextToken token;
                    token.type = TextToken;
                    token.text = text;
                    token.value = 0;
                    token.size = 0;
                    token.operand = 0xffffffff;
                    token.context = NoTokenContext;
                    token.address = 0;
                    token.confidence = 255;
                    line.tokens.push_back(token);
                }

                lineVec.push_back(line);
            }
            node.SetLines(lineVec);
        },

        // Get outgoing edges
        "outgoing_edges", [](sol::this_state ts, FlowGraphNode& node) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            const std::vector<FlowGraphEdge>& edges = node.GetOutgoingEdges();
            for (size_t i = 0; i < edges.size(); i++) {
                sol::table edge = lua.create_table();
                edge["type"] = BranchTypeToString(edges[i].type);
                edge["target"] = edges[i].target;
                edge["back_edge"] = edges[i].backEdge;

                sol::table points = lua.create_table();
                for (size_t j = 0; j < edges[i].points.size(); j++) {
                    sol::table point = lua.create_table();
                    point["x"] = edges[i].points[j].x;
                    point["y"] = edges[i].points[j].y;
                    points[j + 1] = point;
                }
                edge["points"] = points;

                result[i + 1] = edge;
            }
            return result;
        },

        // Get incoming edges
        "incoming_edges", [](sol::this_state ts, FlowGraphNode& node) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            const std::vector<FlowGraphEdge>& edges = node.GetIncomingEdges();
            for (size_t i = 0; i < edges.size(); i++) {
                sol::table edge = lua.create_table();
                edge["type"] = BranchTypeToString(edges[i].type);
                edge["target"] = edges[i].target;
                edge["back_edge"] = edges[i].backEdge;
                result[i + 1] = edge;
            }
            return result;
        },

        // Add outgoing edge
        "add_outgoing_edge", [](FlowGraphNode& node, const std::string& type,
                                FlowGraphNode& target) {
            BNEdgeStyle style;
            style.style = SolidLine;
            style.width = 1;
            style.color = WhiteStandardHighlightColor;
            node.AddOutgoingEdge(StringToBranchType(type), &target, style);
        },

        // Comparison
        sol::meta_function::equal_to, [](FlowGraphNode& a, FlowGraphNode& b) -> bool {
            return FlowGraphNode::GetObject(&a) == FlowGraphNode::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](FlowGraphNode& node) -> std::string {
            return fmt::format("<FlowGraphNode @ ({}, {})>", node.GetX(), node.GetY());
        }
    );

    // FlowGraph usertype
    lua.new_usertype<FlowGraph>(FLOWGRAPH_METATABLE,
        // Constructor creates empty graph
        sol::call_constructor, sol::factories([]() -> Ref<FlowGraph> {
            return new FlowGraph();
        }),

        // Dimensions
        "width", sol::property(&FlowGraph::GetWidth, &FlowGraph::SetWidth),
        "height", sol::property(&FlowGraph::GetHeight, &FlowGraph::SetHeight),

        // Node count
        "node_count", sol::property(&FlowGraph::GetNodeCount),
        "has_nodes", sol::property(&FlowGraph::HasNodes),

        // Associated function/view
        "function", sol::property(
            [](FlowGraph& g) -> Ref<Function> { return g.GetFunction(); },
            [](FlowGraph& g, Ref<Function> f) { g.SetFunction(f); }
        ),
        "view", sol::property(
            [](FlowGraph& g) -> Ref<BinaryView> { return g.GetView(); },
            [](FlowGraph& g, Ref<BinaryView> v) { g.SetView(v); }
        ),

        // IL type checks
        "is_il", sol::property(&FlowGraph::IsILGraph),
        "is_llil", sol::property(&FlowGraph::IsLowLevelILGraph),
        "is_mlil", sol::property(&FlowGraph::IsMediumLevelILGraph),
        "is_hlil", sol::property(&FlowGraph::IsHighLevelILGraph),

        // Layout status
        "is_layout_complete", sol::property(&FlowGraph::IsLayoutComplete),

        // Get all nodes
        "nodes", [](sol::this_state ts, FlowGraph& g) -> sol::table {
            sol::state_view lua(ts);
            sol::table result = lua.create_table();
            std::vector<Ref<FlowGraphNode>> nodes = g.GetNodes();
            for (size_t i = 0; i < nodes.size(); i++) {
                result[i + 1] = nodes[i];
            }
            return result;
        },

        // Get node by index
        "get_node", [](FlowGraph& g, size_t index) -> Ref<FlowGraphNode> {
            return g.GetNode(index);
        },

        // Add node to graph
        "add_node", [](FlowGraph& g, FlowGraphNode& node) -> size_t {
            return g.AddNode(&node);
        },

        // Create a new node for this graph
        "create_node", [](FlowGraph& g) -> Ref<FlowGraphNode> {
            return new FlowGraphNode(&g);
        },

        // Clear all nodes
        "clear_nodes", &FlowGraph::ClearNodes,

        // Comparison
        sol::meta_function::equal_to, [](FlowGraph& a, FlowGraph& b) -> bool {
            return FlowGraph::GetObject(&a) == FlowGraph::GetObject(&b);
        },

        // String representation
        sol::meta_function::to_string, [](FlowGraph& g) -> std::string {
            return fmt::format("<FlowGraph: {} nodes>", g.GetNodeCount());
        }
    );

    // Provide FlowGraph.new() constructor
    lua["FlowGraph"] = lua.create_table_with(
        "new", []() -> Ref<FlowGraph> {
            return new FlowGraph();
        }
    );

    // Also provide FlowGraphNode constructor that takes a graph
    lua["FlowGraphNode"] = lua.create_table_with(
        "new", [](FlowGraph& graph) -> Ref<FlowGraphNode> {
            return new FlowGraphNode(&graph);
        }
    );

    if (logger) logger->LogDebug("FlowGraph bindings registered");
}

} // namespace BinjaLua
