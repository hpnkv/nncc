#pragma once

#include <iostream>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <variant>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/topological_sort.hpp>
#include <entt/entt.hpp>
#include <imnodes/imnodes.h>

#include <nncc/common/types.h>
#include <nncc/common/utils.h>
#include <nncc/context/context.h>

namespace nncc::compute {

enum class AttributeType {
    Float,
    String,
    Tensor,
    Array,
    UserDefined,

    Count,
    None
};

struct Attribute {
    Attribute(nncc::string _name, const AttributeType& _type)
            : id(id_counter++), name(std::move(_name)), type(_type) {}

    nncc::string name = "";
    AttributeType type = AttributeType::None;

    entt::entity entity = entt::null;
    std::variant<float, nncc::string> value;

    int id;
    static int id_counter;

    void Feed(const Attribute& other) {
        value = other.value;
        entity = other.entity;
    };
};


struct Result {
    int code = -1;
    nncc::string message = "Node was not fully evaluated.";
};

struct ComputeNode;

using EvaluateDelegate = entt::delegate<Result(ComputeNode*, entt::registry*)>;
using RenderDelegate = entt::delegate<void(ComputeNode*)>;

struct ComputeNode {
    ComputeNode() : id(id_counter++) {};

    void AddInput(Attribute attribute) {
        inputs_by_name.insert_or_assign(attribute.name, attribute);
        inputs.push_back(attribute.name);
    }

    void AddSetting(Attribute attribute) {
        settings_by_name.insert_or_assign(attribute.name, attribute);
        settings.push_back(attribute.name);
    }

    void AddOutput(Attribute attribute) {
        outputs_by_name.insert_or_assign(attribute.name, attribute);
        outputs.push_back(attribute.name);
    }

    template<class T>
    std::shared_ptr<T> StateAs() {
        return std::static_pointer_cast<T>(state);
    }

    nncc::string type, name;

    int id;
    static int id_counter;

    EvaluateDelegate evaluate;
    RenderDelegate render_ui;
    RenderDelegate render_context_ui;

    nncc::list<nncc::string> inputs;
    std::unordered_map<nncc::string, Attribute> inputs_by_name;

    nncc::list<nncc::string> settings;
    std::unordered_map<nncc::string, Attribute> settings_by_name;

    nncc::list<nncc::string> outputs;
    std::unordered_map<nncc::string, Attribute> outputs_by_name;

    std::shared_ptr<void> state;
};

struct ComputeEdge {
    nncc::string from_output;
    nncc::string to_input;
};


struct ComputeGraph {
public:
    using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, ComputeNode, ComputeEdge>;
    using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
    using Edge = boost::graph_traits<Graph>::edge_descriptor;

    Vertex AddNode(const ComputeNode& node) {
        return boost::add_vertex(node, graph);
    }

    void Evaluate(entt::registry* registry) {
        nncc::vector<Vertex> sorted_vertices;
        boost::topological_sort(graph, std::back_inserter(sorted_vertices));

        for (auto it = sorted_vertices.rbegin(); it != sorted_vertices.rend(); ++it) {
            auto& vertex = *it;
            std::cout << graph[vertex].id << " " << graph[vertex].name << std::endl;
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

    Graph graph {};
};


ComputeNode MakeConstOp(const nncc::string& name);

ComputeNode MakeAddOp(const nncc::string& name);

ComputeNode MakeMulOp(const nncc::string& name);


class ComputeNodeEditor {
public:
    ComputeNodeEditor()
            : graph_(), minimap_location_(ImNodesMiniMapLocation_BottomRight) {
        ImNodesIO& io = ImNodes::GetIO();
        io.LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;

        auto& context = context::Context::Get();
        scale_ = context.GetWindow(0).scale;

        ImGui::StyleColorsLight();
        ImNodes::StyleColorsLight();
    }

    void Update() {
        selected_nodes_.clear();

        auto flags = ImGuiWindowFlags_MenuBar;

        // The node editor window
        ImGui::Begin("Compute node editor", nullptr, flags);

        ShowMenuBar();

        ImGui::TextUnformatted("Edit the color of the output color window using nodes.");
        ImGui::Columns(2);
        ImGui::TextUnformatted("A -- add node");
        ImGui::TextUnformatted("X -- delete selected node or link");
        ImGui::NextColumn();
        ImGui::Columns(1);

        ImNodes::BeginNodeEditor();
        HandleNewNodes();
        RenderNodes();
        RenderLinks();
        ImNodes::MiniMap(0.2f, minimap_location_);
        ImNodes::EndNodeEditor();

        HandleAddedLinks();
        HandleDeletedLinks();
        HandleDeletedNodes();
        selected_nodes_ = CollectSelectedNodes();

        ImGui::End();
    }

    const nncc::vector<ComputeNode*>& GetSelectedNodes() const {
        return selected_nodes_;
    }

    ComputeGraph& GetGraph() {
        return graph_;
    }

private:
    struct AttributeDescriptor {
        ComputeGraph::Vertex vertex;
        Attribute* attribute;
        bool is_input = false;
    };

    float scale_ = 1.0f;

    ComputeGraph graph_;
    std::unordered_map<int, ComputeGraph::Edge> edge_map_;
    std::unordered_map<int, ComputeGraph::Vertex> vertex_map_;
    std::unordered_map<int, AttributeDescriptor> attribute_map_;

    nncc::vector<ComputeNode*> selected_nodes_;
    ImNodesMiniMapLocation minimap_location_;

    void ShowMenuBar() {
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
    }

    void HandleNewNodes() {
        const bool open_popup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                                ImNodes::IsEditorHovered() &&
                                ImGui::IsKeyReleased(ImGuiKey_A);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f * scale_, 8.f * scale_));
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
                auto node = MakeConstOp("Constant");
                graph_.AddNode(node);
                ImNodes::SetNodeScreenSpacePos(node.id, click_pos);
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    void RenderNodes() {
        for (auto [nodes_current, nodes_end] = boost::vertices(*graph_); nodes_current != nodes_end; ++nodes_current) {
            auto vertex = *nodes_current;
            auto& node = (*graph_)[vertex];

            const float node_width = 100.f * scale_;
            ImNodes::BeginNode(node.id);

            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(node.name.c_str());
            ImNodes::EndNodeTitleBar();
            for (auto& [name, input]: node.inputs_by_name) {
                ImNodes::BeginInputAttribute(input.id);
                attribute_map_.insert_or_assign(input.id, AttributeDescriptor{vertex, &input, true});
                const float label_width = ImGui::CalcTextSize(input.name.c_str()).x;
                ImGui::TextUnformatted(input.name.c_str());

                ImGui::SameLine();
                ImGui::PushItemWidth(node_width - label_width);
                if (std::holds_alternative<float>(input.value)) {
                    ImGui::DragFloat("##hidelabel", &std::get<float>(input.value), 0.01f);
                } else if (std::holds_alternative<nncc::string>(input.value)) {
                    ImGui::TextUnformatted(std::get<nncc::string>(input.value).c_str());
                }

                ImGui::PopItemWidth();

                ImNodes::EndInputAttribute();
            }

            ImGui::Spacing();

            for (auto& [name, output]: node.outputs_by_name) {
                {
                    ImNodes::BeginOutputAttribute(output.id);
                    attribute_map_.insert_or_assign(output.id, AttributeDescriptor{vertex, &output, false});
                    nncc::string type;
                    if (output.type == AttributeType::Float) {
                        type = "float";
                    } else if (output.type == AttributeType::String) {
                        type = "string";
                    } else if (output.type == AttributeType::UserDefined) {
                        type = "T";
                    }

                    nncc::string label = !type.empty() ? fmt::format("{}: {}", output.name, type) : output.name;
                    const float label_width = ImGui::CalcTextSize(label.c_str()).x;
                    ImGui::Indent(node_width - label_width);
                    ImGui::TextUnformatted(label.c_str());
                    ImNodes::EndOutputAttribute();
                }
            }

            ImNodes::EndNode();
        }
    }

    void RenderLinks() {
        int edge_id = 0;
        edge_map_.clear();
        auto [begin_edge, end_edge] = boost::edges(*graph_);
        for (auto current_edge = begin_edge; current_edge != end_edge; ++current_edge) {
            const auto& edge = (*graph_)[*current_edge];

            const auto& source_node = (*graph_)[boost::source(*current_edge, *graph_)];
            const auto& target_node = (*graph_)[boost::target(*current_edge, *graph_)];

            auto id = edge_id++;
            ImNodes::Link(id,
                          source_node.outputs_by_name.at(edge.from_output).id,
                          target_node.inputs_by_name.at(edge.to_input).id);
            edge_map_[id] = *current_edge;
        }
    }

    void HandleAddedLinks() {
        int start_attr, end_attr;
        if (!ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
            return;
        }

        auto start = attribute_map_[start_attr];
        auto end = attribute_map_[end_attr];

        if (!start.is_input && end.is_input) {
            boost::add_edge(start.vertex, end.vertex, {start.attribute->name, end.attribute->name}, *graph_);
        }

        graph_.Evaluate(&context::Context::Get().registry);
    }

    void HandleDeletedLinks() {
        int link_id;
        if (ImNodes::IsLinkDestroyed(&link_id)) {
            boost::remove_edge(edge_map_.at(link_id), *graph_);
            edge_map_.erase(link_id);
        }

        const int num_selected = ImNodes::NumSelectedLinks();
        if (num_selected <= 0 || !ImGui::IsKeyReleased(ImGuiKey_X)) {
            return;
        }

        nncc::vector<int> selected_links;
        selected_links.resize(static_cast<size_t>(num_selected));
        ImNodes::GetSelectedLinks(selected_links.data());
        for (const auto& link: selected_links) {
            boost::remove_edge(edge_map_.at(link), *graph_);
            edge_map_.erase(link);
        }
    }

    void HandleDeletedNodes() {
        const int num_selected = ImNodes::NumSelectedNodes();
        if (num_selected <= 0 || !ImGui::IsKeyReleased(ImGuiKey_X)) {
            return;
        }

        nncc::vector<int> selected_nodes;
        selected_nodes.resize(static_cast<size_t>(num_selected));
        ImNodes::GetSelectedNodes(selected_nodes.data());
        for (const auto& node_id: selected_nodes) {
            auto vertex = vertex_map_[node_id];
            const auto& node = (*graph_)[vertex];

            nncc::vector<int> attribute_ids;
            attribute_ids.reserve(node.inputs.size() + node.outputs.size() + node.settings.size());
            for (const auto& [name, attribute]: node.inputs_by_name) {
                attribute_ids.push_back(attribute.id);
            }
            for (const auto& [name, attribute]: node.outputs_by_name) {
                attribute_ids.push_back(attribute.id);
            }
            for (const auto& [name, attribute]: node.settings_by_name) {
                attribute_ids.push_back(attribute.id);
            }
            for (const auto& id: attribute_ids) {
                attribute_map_.erase(id);
            }

            boost::remove_vertex(vertex, *graph_);
        }
    }

    nncc::vector<ComputeNode*> CollectSelectedNodes() {
        const int num_selected_nodes = ImNodes::NumSelectedNodes();
        if (num_selected_nodes <= 0) {
            return {};
        }

        nncc::vector<int> selected_node_ids;
        selected_node_ids.resize(num_selected_nodes);
        ImNodes::GetSelectedNodes(selected_node_ids.data());

        nncc::vector<ComputeNode*> selected_nodes;
        for (const auto& node_id: selected_node_ids) {
            selected_nodes.push_back(&(*graph_)[node_id]);
        }

        return selected_nodes;
    }
};

} // namespace nncc::nodes



