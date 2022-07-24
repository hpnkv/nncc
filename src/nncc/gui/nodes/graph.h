#pragma once

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/topological_sort.hpp>
#include <entt/entt.hpp>

#include <nncc/common/types.h>
#include <nncc/common/utils.h>

namespace nncc::nodes {

enum class AttributeType {
    Scalar,
    String,
    Tensor,
    Array,
    List,
    UserDefined,

    Count,
    None
};

struct Attribute {
    Attribute(nncc::string name) : id(Attribute::id_counter++) {}

    nncc::string name = "";
    AttributeType type = AttributeType::None;

    entt::entity entity = entt::null;
    float value = 0.0f;

    uint32_t id = 0;
    static uint32_t id_counter;

    void Feed(const Attribute& other);
};

struct Result {
    int code = -1;
    nncc::string message = "Node was not fully evaluated.";
};

struct ComputeNode;

using EvaluateDelegate = entt::delegate<Result(ComputeNode*, entt::registry*)>;
using RenderDelegate = entt::delegate<void(ComputeNode*)>;

struct ComputeNode {
    ComputeNode() : id(ComputeNode::id_counter++) {};

    void AddInput(const Attribute& attribute) {
        inputs.push_back(attribute);
        inputs_by_name.insert_or_assign(attribute.name, inputs.back());
    }

    void AddSetting(const Attribute& attribute) {
        settings.push_back(attribute);
        settings_by_name.insert_or_assign(attribute.name, settings.back());
    }

    void AddOutput(const Attribute& attribute) {
        outputs.push_back(attribute);
        outputs_by_name.insert_or_assign(attribute.name, outputs.back());
    }

    nncc::string type, name;

    uint32_t id;
    static uint32_t id_counter;

    EvaluateDelegate evaluate;
    RenderDelegate render_ui;
    RenderDelegate render_extended_view;

    nncc::list<Attribute> inputs;
    std::unordered_map<nncc::string, Attribute&> inputs_by_name;

    nncc::list<Attribute> settings;
    std::unordered_map<nncc::string, Attribute&> settings_by_name;

    nncc::list<Attribute> outputs;
    std::unordered_map<nncc::string, Attribute&> outputs_by_name;
};

uint32_t Attribute::id_counter = 0;
uint32_t ComputeNode::id_counter = 0;

struct ComputeEdge {
    nncc::string from_output;
    nncc::string to_input;
};

struct ComputeGraph {
public:
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, ComputeNode, ComputeEdge>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;

    Vertex AddNode(const ComputeNode& node) {
        return boost::add_vertex(node, graph);
    }

    void Evaluate(entt::registry* registry) {
        nncc::vector<Vertex> sorted_vertices;
        boost::topological_sort(graph, std::back_inserter(sorted_vertices));

        for (const auto& vertex : sorted_vertices) {
            graph[vertex].evaluate(&graph[vertex], registry);
            for (auto [current, end] = boost::adjacent_vertices(vertex, graph); current != end; ++current) {
                auto edge = boost::edge(vertex, *current, graph).first;

                auto source = boost::source(edge, graph), target = boost::target(edge, graph);
                auto from = graph[edge].from_output, to = graph[edge].to_input;

                graph[target].inputs_by_name.at(to).Feed(graph[source].outputs_by_name.at(from));
            }
        }
    }

    const Graph& operator*() const {
        return graph;
    };

    Graph& operator*() {
        return graph;
    };

    Graph graph;
};

ComputeNode MakeConstOp(const nncc::string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Const";

    node.AddSetting({.name = "value", .type = AttributeType::UserDefined});
    node.AddOutput({.name = "value", .type = AttributeType::UserDefined});

    auto evaluate_fn = [] (ComputeNode* node, entt::registry* registry) {
        node->outputs_by_name.at("value").value = node->settings_by_name.at("value").value;
        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    return node;
}

ComputeNode MakeAddOp(const nncc::string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Add";

    node.AddInput({.name = "a", .type = AttributeType::UserDefined});
    node.AddInput({.name = "b", .type = AttributeType::UserDefined});

    node.AddOutput({.name = "result", .type = AttributeType::UserDefined});

    auto evaluate_fn = [] (ComputeNode* node, entt::registry* registry) {
        auto a = node->inputs_by_name.at("a").value;
        auto b = node->inputs_by_name.at("b").value;

        node->outputs_by_name.at("result").value = a + b;

        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    return node;
}

ComputeNode MakeMulOp(const nncc::string& name) {
    ComputeNode node;

    node.name = name;
    node.type = "Mul";

    node.AddInput({.name = "a", .type = AttributeType::UserDefined});
    node.AddInput({.name = "b", .type = AttributeType::UserDefined});

    node.AddOutput({.name = "result", .type = AttributeType::UserDefined});

    auto evaluate_fn = [] (ComputeNode* node, entt::registry* registry) {
        auto a = node->inputs_by_name.at("a").value;
        auto b = node->inputs_by_name.at("b").value;

        node->outputs_by_name.at("result").value = a + b;

        return Result{0, ""};
    };
    node.evaluate.connect<evaluate_fn>();

    return node;
}

#include <imnodes/imnodes.h>

static float current_time_seconds = 0.0f;

class ColorNodeEditor {
public:
    ColorNodeEditor()
            : graph_(), minimap_location_(ImNodesMiniMapLocation_BottomRight) {
    }

    void show(float seconds) {
        // Update timer context
        current_time_seconds = seconds;

        auto flags = ImGuiWindowFlags_MenuBar;

        // The node editor window
        ImGui::Begin("color node editor", nullptr, flags);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Mini-map")) {
                const char* names[] = {
                        "Top Left",
                        "Top Right",
                        "Bottom Left",
                        "Bottom Right",
                };
                int locations[] = {
                        ImNodesMiniMapLocation_TopLeft,
                        ImNodesMiniMapLocation_TopRight,
                        ImNodesMiniMapLocation_BottomLeft,
                        ImNodesMiniMapLocation_BottomRight,
                };

                for (int i = 0; i < 4; i++) {
                    bool selected = minimap_location_ == locations[i];
                    if (ImGui::MenuItem(names[i], nullptr, &selected))
                        minimap_location_ = locations[i];
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Style")) {
                if (ImGui::MenuItem("Classic")) {
                    ImGui::StyleColorsClassic();
                    ImNodes::StyleColorsClassic();
                }
                if (ImGui::MenuItem("Dark")) {
                    ImGui::StyleColorsDark();
                    ImNodes::StyleColorsDark();
                }
                if (ImGui::MenuItem("Light")) {
                    ImGui::StyleColorsLight();
                    ImNodes::StyleColorsLight();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::TextUnformatted("Edit the color of the output color window using nodes.");
        ImGui::Columns(2);
        ImGui::TextUnformatted("A -- add node");
        ImGui::TextUnformatted("X -- delete selected node or link");
        ImGui::NextColumn();
        ImGui::Columns(1);

        ImNodes::BeginNodeEditor();

        // Handle new nodes
        // These are driven by the user, so we place this code before rendering the nodes
        {
            const bool open_popup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                                    ImNodes::IsEditorHovered() &&
                                    ImGui::IsKeyReleased(ImGuiKey_A);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
            if (!ImGui::IsAnyItemHovered() && open_popup) {
                ImGui::OpenPopup("Add node");
            }

            if (ImGui::BeginPopup("Add node")) {
                const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

                if (ImGui::MenuItem("Add")) {
                    auto node = MakeAddOp("Add");
                    graph_.AddNode(node);
                    ImNodes::SetNodeScreenSpacePos(node.id, click_pos);
                }

                if (ImGui::MenuItem("Multiply")) {
                    auto node = MakeMulOp("Multiply");
                    graph_.AddNode(node);
                    ImNodes::SetNodeScreenSpacePos(node.id, click_pos);
                }

                if (ImGui::MenuItem("Constant")) {
                    auto node = MakeConstOp("Multiply");
                    graph_.AddNode(node);
                    ImNodes::SetNodeScreenSpacePos(node.id, click_pos);
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();
        }

        for (auto [nodes_current, nodes_end] = boost::vertices(*graph_); nodes_current != nodes_end; ++nodes_current) {
            auto& node = (*graph_)[*nodes_current];
            if (node.type == "Add") {
                const float node_width = 100.f;
                ImNodes::BeginNode(node.id);

                ImNodes::BeginNodeTitleBar();
                ImGui::TextUnformatted(node.name.c_str());
                ImNodes::EndNodeTitleBar();
                {
                    ImNodes::BeginInputAttribute(node.ui.add.lhs);
                    const float label_width = ImGui::CalcTextSize("left").x;
                    ImGui::TextUnformatted("left");
                    if (graph_.num_edges_from_node(node.ui.add.lhs) == 0ull) {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat("##hidelabel", &graph_.node(node.ui.add.lhs).value, 0.01f);
                        ImGui::PopItemWidth();
                    }
                    ImNodes::EndInputAttribute();
                }

                {
                    ImNodes::BeginInputAttribute(node.ui.add.rhs);
                    const float label_width = ImGui::CalcTextSize("right").x;
                    ImGui::TextUnformatted("right");
                    if (graph_.num_edges_from_node(node.ui.add.rhs) == 0ull) {
                        ImGui::SameLine();
                        ImGui::PushItemWidth(node_width - label_width);
                        ImGui::DragFloat("##hidelabel", &graph_.node(node.ui.add.rhs).value, 0.01f);
                        ImGui::PopItemWidth();
                    }
                    ImNodes::EndInputAttribute();
                }

                ImGui::Spacing();

                {
                    ImNodes::BeginOutputAttribute(node.id);
                    const float label_width = ImGui::CalcTextSize("result").x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted("result");
                    ImNodes::EndOutputAttribute();
                }

                ImNodes::EndNode();
            }

            switch (node.type) {
                case UiNodeType::add: {

                }
                    break;
                case UiNodeType::multiply: {
                    const float node_width = 100.0f;
                    ImNodes::BeginNode(node.id);

                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted("multiply");
                    ImNodes::EndNodeTitleBar();

                    {
                        ImNodes::BeginInputAttribute(node.ui.multiply.lhs);
                        const float label_width = ImGui::CalcTextSize("left").x;
                        ImGui::TextUnformatted("left");
                        if (graph_.num_edges_from_node(node.ui.multiply.lhs) == 0ull) {
                            ImGui::SameLine();
                            ImGui::PushItemWidth(node_width - label_width);
                            ImGui::DragFloat(
                                    "##hidelabel", &graph_.node(node.ui.multiply.lhs).value, 0.01f);
                            ImGui::PopItemWidth();
                        }
                        ImNodes::EndInputAttribute();
                    }

                    {
                        ImNodes::BeginInputAttribute(node.ui.multiply.rhs);
                        const float label_width = ImGui::CalcTextSize("right").x;
                        ImGui::TextUnformatted("right");
                        if (graph_.num_edges_from_node(node.ui.multiply.rhs) == 0ull) {
                            ImGui::SameLine();
                            ImGui::PushItemWidth(node_width - label_width);
                            ImGui::DragFloat(
                                    "##hidelabel", &graph_.node(node.ui.multiply.rhs).value, 0.01f);
                            ImGui::PopItemWidth();
                        }
                        ImNodes::EndInputAttribute();
                    }

                    ImGui::Spacing();

                    {
                        ImNodes::BeginOutputAttribute(node.id);
                        const float label_width = ImGui::CalcTextSize("result").x;
                        ImGui::Indent(node_width - label_width);
                        ImGui::TextUnformatted("result");
                        ImNodes::EndOutputAttribute();
                    }

                    ImNodes::EndNode();
                }
                    break;
            }
        }

        for (const auto& edge : graph_.edges()) {
            // If edge doesn't start at value, then it's an internal edge, i.e.
            // an edge which links a node's operation to its input. We don't
            // want to render node internals with visible links.
            if (graph_.node(edge.from).type != NodeType::value)
                continue;

            ImNodes::Link(edge.id, edge.from, edge.to);
        }

        ImNodes::MiniMap(0.2f, minimap_location_);
        ImNodes::EndNodeEditor();

        // Handle new links
        // These are driven by Imnodes, so we place the code after EndNodeEditor().

        {
            int start_attr, end_attr;
            if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
                const NodeType start_type = graph_.node(start_attr).type;
                const NodeType end_type = graph_.node(end_attr).type;

                const bool valid_link = start_type != end_type;
                if (valid_link) {
                    // Ensure the edge is always directed from the value to
                    // whatever produces the value
                    if (start_type != NodeType::value) {
                        std::swap(start_attr, end_attr);
                    }
                    graph_.insert_edge(start_attr, end_attr);
                }
            }
        }

        // Handle deleted links

        {
            int link_id;
            if (ImNodes::IsLinkDestroyed(&link_id)) {
                graph_.erase_edge(link_id);
            }
        }

        {
            const int num_selected = ImNodes::NumSelectedLinks();
            if (num_selected > 0 && ImGui::IsKeyReleased(ImGuiKey_X)) {
                static std::vector<int> selected_links;
                selected_links.resize(static_cast<size_t>(num_selected));
                ImNodes::GetSelectedLinks(selected_links.data());
                for (const int edge_id : selected_links) {
                    graph_.erase_edge(edge_id);
                }
            }
        }

        {
            const int num_selected = ImNodes::NumSelectedNodes();
            if (num_selected > 0 && ImGui::IsKeyReleased(ImGuiKey_X)) {
                static std::vector<int> selected_nodes;
                selected_nodes.resize(static_cast<size_t>(num_selected));
                ImNodes::GetSelectedNodes(selected_nodes.data());
                for (const int node_id : selected_nodes) {
                    graph_.erase_node(node_id);
                    auto iter = std::find_if(
                            nodes_.begin(), nodes_.end(), [node_id](const UiNode& node) -> bool {
                                return node.id == node_id;
                            });
                    // Erase any additional internal nodes
                    switch (iter->type) {
                        case UiNodeType::add:graph_.erase_node(iter->ui.add.lhs);
                            graph_.erase_node(iter->ui.add.rhs);
                            break;
                        case UiNodeType::multiply:graph_.erase_node(iter->ui.multiply.lhs);
                            graph_.erase_node(iter->ui.multiply.rhs);
                            break;
                        case UiNodeType::output:graph_.erase_node(iter->ui.output.r);
                            graph_.erase_node(iter->ui.output.g);
                            graph_.erase_node(iter->ui.output.b);
                            root_node_id_ = -1;
                            break;
                        case UiNodeType::sine:graph_.erase_node(iter->ui.sine.input);
                            break;
                        default:break;
                    }
                    nodes_.erase(iter);
                }
            }
        }

        ImGui::End();

        // The color output window

        const ImU32 color =
                root_node_id_ != -1 ? evaluate(graph_, root_node_id_) : IM_COL32(255, 20, 147, 255);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, color);
        ImGui::Begin("output color");
        ImGui::End();
        ImGui::PopStyleColor();
    }

private:
    ComputeGraph graph_;
    int root_node_id_;
    ImNodesMiniMapLocation minimap_location_;
};


ColorNodeEditor color_editor;
} // namespace

void NodeEditorInitialize() {
    ImNodesIO& io = ImNodes::GetIO();
    io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditorShow(float seconds) { color_editor.show(seconds); }

void NodeEditorShutdown() {}



}