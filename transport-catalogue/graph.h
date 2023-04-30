#pragma once

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

#include "ranges.h"

namespace graph {

    using VertexId = size_t;
    using EdgeId = size_t;

    template <typename Weight>
    struct Edge {
        VertexId from;
        VertexId to;
        Weight weight;
    };

    template <typename Weight>
    class DirectedWeightedGraph {
    public:
        using WeightType = Weight;
        using EdgeType = Edge<Weight>;
        using IncidenceList = std::vector<EdgeId>;
        using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;
        using EdgeContainer = std::vector<Edge<Weight>>;
        using IncidentEdges = std::vector<IncidenceList>;

    public:
        DirectedWeightedGraph() = default;
        explicit DirectedWeightedGraph(size_t vertex_count);

        template <typename TEdgeContainer, std::enable_if_t<std::is_same_v<std::decay_t<TEdgeContainer>, EdgeContainer>, bool> = true>
        DirectedWeightedGraph(TEdgeContainer&& edges, size_t vertex_count);

        template <
            typename TEdgeContainer, typename TIncidenceListContainer,
            std::enable_if_t<
                std::is_same_v<std::decay_t<TEdgeContainer>, EdgeContainer> && std::is_same_v<TIncidenceListContainer, IncidentEdges>,
                bool> = true>
        DirectedWeightedGraph(TEdgeContainer&& edges, TIncidenceListContainer incidence_lists);

        template <typename TEdge, std::enable_if_t<std::is_same_v<std::decay_t<TEdge>, Edge<Weight>>, bool> = true>
        EdgeId AddEdge(TEdge&& edge);

        size_t GetVertexCount() const;
        size_t GetEdgeCount() const;
        const Edge<Weight>& GetEdge(EdgeId edge_id) const;
        IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

    private:
        EdgeContainer edges_;
        IncidentEdges incidence_lists_;
    };

    template <typename Weight>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count) : incidence_lists_(vertex_count) {}

    template <typename Weight>
    template <
        typename TEdgeContainer,
        std::enable_if_t<std::is_same_v<std::decay_t<TEdgeContainer>, typename DirectedWeightedGraph<Weight>::EdgeContainer>, bool>>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(TEdgeContainer&& edges, size_t vertex_count)
        : DirectedWeightedGraph(vertex_count), edges_(std::forward<TEdgeContainer>(edges)) {
        std::for_each(std::move_iterator(edges_.begin()), std::move_iterator(edges_.end()), [&](auto&& edge) {
            AddEdge(std::forward<decltype(edge)>(edge));
        });
    }

    template <typename Weight>
    template <
        typename TEdgeContainer, typename TIncidenceListContainer,
        std::enable_if_t<
            std::is_same_v<std::decay_t<TEdgeContainer>, typename DirectedWeightedGraph<Weight>::EdgeContainer> &&
                std::is_same_v<TIncidenceListContainer, typename DirectedWeightedGraph<Weight>::IncidentEdges>,
            bool>>
    DirectedWeightedGraph<Weight>::DirectedWeightedGraph(TEdgeContainer&& edges, TIncidenceListContainer incidence_lists)
        :  edges_{std::forward<TEdgeContainer>(edges)}, incidence_lists_{std::forward<TIncidenceListContainer>(incidence_lists)} {}

    template <typename Weight>
    template <typename TEdge, std::enable_if_t<std::is_same_v<std::decay_t<TEdge>, Edge<Weight>>, bool>>
    EdgeId DirectedWeightedGraph<Weight>::AddEdge(TEdge&& edge) {
        const Edge<Weight>& emplaced_edge = edges_.emplace_back(std::forward<TEdge>(edge));
        const EdgeId id = edges_.size() - 1;
        incidence_lists_.at(emplaced_edge.from).push_back(id);
        return id;
    }

    template <typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
        return incidence_lists_.size();
    }

    template <typename Weight>
    size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
        return edges_.size();
    }

    template <typename Weight>
    const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
        return edges_.at(edge_id);
    }

    template <typename Weight>
    typename DirectedWeightedGraph<Weight>::IncidentEdgesRange DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
        return ranges::AsRange(incidence_lists_.at(vertex));
    }
}