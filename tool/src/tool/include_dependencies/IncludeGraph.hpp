// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DependencyAnalysis.hpp"
#include "IncludeEdge.hpp"
#include "IncludeNode.hpp"

#include "../frame/ProjectFiles.hpp"
#include "../headers/PublicHeaderMap.hpp"

#include <erbsland/json/JsonValue.hpp>
#include <erbsland/List.hpp>
#include <erbsland/StringHashMap.hpp>
#include <erbsland/StringHashSet.hpp>

#include <optional>

namespace tool::include_dependencies {

using namespace el::text::literals;

/// Immutable source include graph with shared forward, reverse, and component indexes.
class IncludeGraph final {
    /// Summary count of inventoried library files.
    static constexpr auto cInternalNodesKey = "internal_nodes"_el;
    /// Summary count of published regular headers.
    static constexpr auto cPublicHeadersKey = "public_headers"_el;
    /// Summary count of resolved quoted includes.
    static constexpr auto cInternalEdgesKey = "internal_edges"_el;
    /// Summary count of angle-bracket include directives.
    static constexpr auto cExternalEdgesKey = "external_edges"_el;
    /// Summary count of unique external include names.
    static constexpr auto cExternalHeadersKey = "external_headers"_el;
    /// Cycle count in summaries and component list in full reports.
    static constexpr auto cCyclesKey = "cycles"_el;
    /// Summary counts grouped by source role.
    static constexpr auto cRolesKey = "roles"_el;
    /// Canonical node identifier in serialized reports.
    static constexpr auto cPathKey = "path"_el;
    /// Serialized internal or external node category.
    static constexpr auto cKindKey = "kind"_el;
    /// Serialized source role such as header or template.
    static constexpr auto cRoleKey = "role"_el;
    /// Serialized regular public-header membership.
    static constexpr auto cPublicKey = "public"_el;

public:
    /// Inventory and parse library sources once.
    /// @param files Confined project file service.
    /// @param sourceRoot Checked source directory.
    /// @param mapping Validated publication map.
    /// @throws el::ApplicationError If a source cannot be parsed or a quoted include cannot be resolved.
    IncludeGraph(const frame::ProjectFiles &files, const el::Path &sourceRoot, const headers::PublicHeaderMap &mapping);

public: // queries
    /// Resolve a complete path or an unambiguous component suffix.
    /// @param identifier User-supplied path, optionally prefixed by src/erbsland/.
    /// @return Canonical source-relative POSIX path.
    /// @throws el::ApplicationError If no unique source matches.
    [[nodiscard]] auto resolve(const el::String &identifier) const -> el::String;
    /// Get the reachable internal files, excluding the starting file itself.
    /// @param path Canonical source-relative path.
    /// @param reverse Follow dependents instead of dependencies.
    /// @return Unique reachable paths.
    [[nodiscard]] auto reachable(const el::String &path, bool reverse = false) const -> el::StringHashSet;
    /// Find a deterministic shortest include chain, including both endpoints.
    /// @param source Canonical source path.
    /// @param target Canonical target path.
    /// @return The chain, or an empty list when no path exists.
    [[nodiscard]] auto shortestPath(const el::String &source, const el::String &target) const -> el::StringList;
    /// Get all cyclic components, including self-includes.
    [[nodiscard]] auto cycles() const -> el::List<el::StringList>;
    /// Find missing public dependencies and minimal insertion suggestions.
    /// @param target Canonical dependency target.
    /// @param excluded Canonical public headers excluded from this rule.
    /// @return Missing headers and leaf-component insertion points.
    [[nodiscard]] auto analyze(const el::String &target, const el::StringHashSet &excluded) const -> DependencyAnalysis;

public: // reports
    /// Render summary counts for human output.
    [[nodiscard]] auto summaryText() const -> el::String;
    /// Render dependency edges with physical line numbers and traversal markers.
    /// @param root Canonical starting file.
    /// @param reverse Follow incoming edges.
    /// @param maxDepth Optional nonnegative traversal depth limit.
    /// @return A deterministic tree ending in a newline.
    [[nodiscard]] auto tree(const el::String &root, bool reverse, std::optional<el::ItemCount> maxDepth) const
        -> el::String;
    /// Serialize schema version 1 as pretty JSON.
    [[nodiscard]] auto toJson() const -> el::String;
    /// Serialize the complete graph as Graphviz DOT.
    [[nodiscard]] auto toDot() const -> el::String;

private:
    /// Parse actual preprocessor directives in one immutable source snapshot.
    void scan(const el::String &path, const el::String &text, const el::Path &sourceRoot);
    /// Create adjacency indexes and strongly connected components without recursive traversal.
    void buildIndexes();
    /// Get unique sorted adjacent internal paths.
    [[nodiscard]] auto adjacent(const el::String &path, bool reverse = false) const -> el::StringList;
    /// Create the schema summary object.
    [[nodiscard]] auto summary() const -> el::json::JsonValue;
    /// Infer the source role using current umbrella and forward-header conventions.
    [[nodiscard]] static auto role(const el::Path &path) -> el::String;
    /// Sort paths case-insensitively with a deterministic binary tie-break.
    static auto less(const el::String &left, const el::String &right) -> bool;
    /// Quote one Graphviz string.
    [[nodiscard]] static auto dotQuote(const el::String &text) -> el::String;

private:
    el::StringList _paths;                             ///< Sorted canonical source paths.
    el::StringHashMap<IncludeNode> _nodes;             ///< Node metadata by exact path.
    el::List<IncludeEdge> _edges;                      ///< Directives in sorted source and physical line order.
    el::StringHashMap<el::List<IncludeEdge>> _forward; ///< All outgoing edges.
    el::StringHashMap<el::List<IncludeEdge>> _reverse; ///< Incoming internal edges.
    el::StringHashMap<el::StringList> _dependencies;   ///< Unique sorted internal dependencies.
    el::StringHashMap<el::StringList> _dependents;     ///< Unique sorted internal dependents.
    el::List<el::StringList> _components;              ///< All SCCs in source-to-sink order.
    el::StringHashMap<el::ItemIndex> _componentIndex;  ///< Component containing each path.
    el::StringList _external;                          ///< Sorted unique external include names.
};

}
