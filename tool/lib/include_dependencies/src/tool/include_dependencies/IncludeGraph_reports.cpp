// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IncludeGraph.hpp"

#include <erbsland/json/JsonContainers.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringMap.hpp>

namespace tool::include_dependencies {

using namespace el::text::literals;

auto IncludeGraph::summary() const -> el::json::JsonValue {
    auto roles = el::StringMap<int64_t>{};
    int64_t publicCount = 0;
    int64_t internalCount = 0;
    for (const auto &[path, node] : _nodes) {
        roles.set(node.role, roles.get(node.role, 0) + 1);
        if (node.isPublic) {
            ++publicCount;
        }
    }
    for (const auto &edge : _edges) {
        if (edge.internal) {
            ++internalCount;
        }
    }
    auto roleValues = el::json::JsonValue{el::json::JsonObject{}};
    for (const auto &[role, count] : roles) {
        roleValues.set(role, count);
    }
    return el::json::JsonValue{el::json::JsonObject{}}
        .set(cInternalNodesKey, _paths.count().toRawValue())
        .set(cPublicHeadersKey, publicCount)
        .set(cInternalEdgesKey, internalCount)
        .set(cExternalEdgesKey, static_cast<int64_t>(_edges.count().toRawValue()) - internalCount)
        .set(cExternalHeadersKey, _external.count().toRawValue())
        .set(cCyclesKey, cycles().count().toRawValue())
        .set(cRolesKey, roleValues);
}

auto IncludeGraph::summaryText() const -> el::String {
    const auto value = summary();
    auto output = el::StringEditor{};
    output.append(
        el::StringFormat{"Internal nodes: {}\nPublic regular headers: {}\nInclude edges: {} internal, {} external\n"
                         "Unique external headers: {}\nInclude cycles: {}\n"_el}
            .build(
                value.get(cInternalNodesKey).getOrThrow<int64_t>(),
                value.get(cPublicHeadersKey).getOrThrow<int64_t>(),
                value.get(cInternalEdgesKey).getOrThrow<int64_t>(),
                value.get(cExternalEdgesKey).getOrThrow<int64_t>(),
                value.get(cExternalHeadersKey).getOrThrow<int64_t>(),
                value.get(cCyclesKey).getOrThrow<int64_t>()));
    const auto roles = value.get(cRolesKey).getOrThrow<el::json::JsonObject>();
    output.append("Roles:\n"_el);
    for (const auto &[role, count] : roles) {
        output.append(el::StringFormat{"  {}: {}\n"_el}.build(role, count.getOrThrow<int64_t>()));
    }
    return output;
}

auto IncludeGraph::tree(const el::String &root, const bool reverse, const std::optional<el::ItemCount> maxDepth) const
    -> el::String {
    auto output = el::StringEditor{};
    output.append(root).append("\n"_el);
    auto seen = el::StringHashSet{};
    auto active = el::StringHashSet{};
    seen.insert(root);
    active.insert(root);
    auto stack = el::List<std::pair<el::String, el::ItemIndex>>{};
    stack.append({root, {}});
    const auto edgesFor = [&](const el::String &path) -> el::List<IncludeEdge> {
        if (reverse) {
            return _reverse.get(path, {});
        }
        return _forward.get(path, {});
    };
    const auto indent = [&](const el::ItemCount count) -> void {
        count.forEach([&](el::ItemIndex) -> void { output.append("  "_el); });
    };
    while (!stack.isEmpty()) {
        auto [path, index] = stack.last();
        const auto depth = stack.count() - el::ItemCount{1};
        const auto edges = edgesFor(path);
        if (maxDepth && depth >= *maxDepth) {
            if (!edges.isEmpty()) {
                indent(stack.count());
                output.append("... [max depth]\n"_el);
            }
            active.remove(path);
            stack.removeLast();
            continue;
        }
        if (!index.isWithin(edges.count())) {
            active.remove(path);
            stack.removeLast();
            continue;
        }
        stack.set(el::ItemIndex::last(stack.count()), {path, index + el::ItemCount{1}});
        const auto edge = edges.get(index);
        auto target = edge.target;
        if (reverse) {
            target = edge.source;
        }
        indent(stack.count());
        if (!edge.internal) {
            output.append("<"_el);
        }
        output.append(target);
        if (!edge.internal) {
            output.append(">"_el);
        }
        output.append(el::StringFormat{":{}"_el}.build(edge.line.toRawValue() + 1));
        auto visit = edge.internal;
        if (edge.internal && active.contains(target)) {
            output.append(" [cycle]"_el);
            visit = false;
        } else if (edge.internal && seen.contains(target)) {
            output.append(" [seen]"_el);
            visit = false;
        }
        output.append("\n"_el);
        if (visit) {
            seen.insert(target);
            active.insert(target);
            stack.append({target, {}});
        }
    }
    return output;
}

auto IncludeGraph::toJson() const -> el::String {
    auto nodes = el::json::JsonValue{el::json::JsonArray{}};
    for (const auto &path : _paths) {
        const auto node = _nodes.get(path, {});
        nodes.append(
            el::json::JsonValue{el::json::JsonObject{}}
                .set(cPathKey, path)
                .set(cKindKey, "internal"_el)
                .set(cRoleKey, node.role)
                .set(cPublicKey, node.isPublic)
                .set("direct_internal_dependencies"_el, adjacent(path).count().toRawValue())
                .set("transitive_internal_dependencies"_el, reachable(path).count().toRawValue())
                .set("direct_internal_dependents"_el, adjacent(path, true).count().toRawValue())
                .set("transitive_internal_dependents"_el, reachable(path, true).count().toRawValue()));
    }
    for (const auto &path : _external) {
        nodes.append(
            el::json::JsonValue{el::json::JsonObject{}}
                .set(cPathKey, el::join("<"_el, path, ">"_el))
                .set(cKindKey, "external"_el)
                .set(cRoleKey, "external"_el)
                .set(cPublicKey, false));
    }
    auto edges = el::json::JsonValue{el::json::JsonArray{}};
    for (const auto &edge : _edges) {
        auto target = edge.target;
        if (!edge.internal) {
            target = el::join("<"_el, target, ">"_el);
        }
        edges.append(
            el::json::JsonValue{el::json::JsonObject{}}
                .set("source"_el, edge.source)
                .set("target"_el, target)
                .set("line"_el, edge.line.toRawValue() + 1)
                .set("internal"_el, edge.internal)
                .set("system"_el, !edge.internal));
    }
    auto components = el::json::JsonValue{el::json::JsonArray{}};
    for (const auto &component : cycles()) {
        auto paths = el::json::JsonValue{el::json::JsonArray{}};
        for (const auto &path : component) {
            paths.append(path);
        }
        components.append(paths);
    }
    const auto document = el::json::JsonValue{el::json::JsonObject{}}
                              .set("schema"_el, 1)
                              .set("summary"_el, summary())
                              .set("nodes"_el, nodes)
                              .set("edges"_el, edges)
                              .set(cCyclesKey, components);
    return el::join(document.toString(el::json::JsonFormatOptions::pretty()), "\n"_el);
}

auto IncludeGraph::dotQuote(const el::String &text) -> el::String {
    return el::join(
        "\""_el,
        text.replacedAll("\\"_el, "\\\\"_el)
            .replacedAll("\""_el, "\\\""_el)
            .replacedAll("\n"_el, "\\n"_el)
            .replacedAll("\r"_el, "\\r"_el),
        "\""_el);
}

auto IncludeGraph::toDot() const -> el::String {
    auto output = el::StringEditor{"digraph include_dependencies {\n  rankdir=LR;\n"_el};
    for (const auto &path : _paths) {
        output.append("  "_el).append(dotQuote(path)).append(" [label="_el).append(dotQuote(path));
        if (_nodes.get(path, {}).isPublic) {
            output.append(", shape=\"box\""_el);
        }
        output.append("];\n"_el);
    }
    for (const auto &path : _external) {
        const auto id = dotQuote(el::join("<"_el, path, ">"_el));
        output.append("  "_el)
            .append(id)
            .append(" [label="_el)
            .append(id)
            .append(", shape=\"ellipse\", style=\"dashed\"];\n"_el);
    }
    for (const auto &edge : _edges) {
        auto target = edge.target;
        if (!edge.internal) {
            target = el::join("<"_el, target, ">"_el);
        }
        output.append(
            el::StringFormat{"  {} -> {} [label=\"{}\"];\n"_el}.build(
                dotQuote(edge.source), dotQuote(target), edge.line.toRawValue() + 1));
    }
    output.append("}\n"_el);
    return output;
}

}
