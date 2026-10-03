// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IncludeGraph.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CodeLocation.hpp>
#include <erbsland/cpp/CppTokenizer.hpp>
#include <erbsland/LineCount.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathError.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathWalker.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::include_dependencies {

using namespace el::text::literals;

IncludeGraph::IncludeGraph(
    const frame::ProjectFiles &files, const el::Path &sourceRoot, const headers::PublicHeaderMap &mapping) {
    auto inputs = el::StringHashMap<el::String>{};
    sourceRoot.walker().walkOrThrow(
        [&](const el::Path &path, const el::PathInfo &info) -> el::PathWalkStatus {
            if (info.isRegularFile()) {
                const auto suffix = path.suffix();
                if (suffix == ".hpp"_el || suffix == ".tpp"_el || suffix == ".cpp"_el) {
                    const auto name = path.toRelativeOrThrow(sourceRoot).toPosix();
                    const auto kind = role(path);
                    _paths.append(name);
                    _nodes.set(
                        name, IncludeNode{name, kind, kind == "header"_el && mapping.canonical().contains(name)});
                    try {
                        inputs.set(name, files.readWalkedTextFile(path).text());
                    } catch (const el::ApplicationError &error) {
                        throw error.withSourcePath(path.toString());
                    }
                }
            }
            return el::PathWalkStatus::Continue;
        },
        el::PathWalkOptions{}.setSymlinkMode(el::SymlinkMode::Skip));
    _paths.sort(less);
    for (const auto &path : _paths) {
        scan(path, inputs.get(path, {}), sourceRoot);
    }
    buildIndexes();
}

auto IncludeGraph::role(const el::Path &path) -> el::String {
    const auto name = path.name();
    if (name == "all.hpp"_el || (name.startsWith("all_"_el) && name.endsWith(".hpp"_el))) {
        return "generated"_el;
    }
    if (name == "fwd.hpp"_el || name.endsWith("_fwd.hpp"_el)) {
        return "forward"_el;
    }
    if (path.suffix() == ".tpp"_el) {
        return "template"_el;
    }
    if (path.suffix() == ".cpp"_el) {
        return "source"_el;
    }
    return "header"_el;
}

auto IncludeGraph::less(const el::String &left, const el::String &right) -> bool {
    const auto folded = left.compare(right, el::Char::compareCaseFolded);
    if (folded != 0) {
        return folded < 0;
    }
    return left.compareBinary(right) < 0;
}

void IncludeGraph::scan(const el::String &path, const el::String &text, const el::Path &sourceRoot) {
    auto counted = el::ByteIndex{};
    auto line = el::LineIndex{};
    try {
        for (const auto &token : el::cpp::CppTokenizer::tokens(text)) {
            if (token.kind() != el::cpp::CppToken::Kind::Directive) {
                continue;
            }
            line += el::LineCount::fromInteger(
                text.slice(el::ByteRange{counted, token.position()}).count("\n"_el).toRawValue());
            counted = token.position();
            const auto directive = token.text()
                                       .slice(el::ByteRange::fromBegin(el::ByteIndex{1}))
                                       .replacedAll("\\\r\n"_el, ""_el)
                                       .replacedAll("\\\n"_el, ""_el);
            auto tokens = el::cpp::CppTokenizer{directive};
            auto next = [&]() -> el::cpp::CppToken {
                try {
                    auto value = tokens.next();
                    while (value.kind() == el::cpp::CppToken::Kind::Comment) {
                        value = tokens.next();
                    }
                    return value;
                } catch (const el::ParseError &) {
                    throw el::ApplicationError{
                        "Invalid Include Directive"_el, "Directive tokenization failed."_el, std::current_exception()};
                }
            };
            if (next().text() != "include"_el) {
                continue;
            }
            const auto targetToken = next();
            auto target = el::String{};
            auto internal = false;
            if (targetToken.kind() == el::cpp::CppToken::Kind::Literal && targetToken.text().startsWith("\""_el)) {
                internal = true;
                target =
                    targetToken.text().slice(el::ByteRange{el::ByteIndex{1}, targetToken.length() - el::ByteCount{2}});
            } else if (targetToken.text() == "<"_el) {
                const auto begin = targetToken.range().endIndex();
                const auto end = directive.find(">"_el, begin);
                if (end.isNoIndex()) {
                    throw el::ApplicationError{
                        "Invalid Include Directive"_el, "The angle-bracket include is unterminated."_el};
                }
                target = directive.slice(el::ByteRange{begin, end});
            } else {
                // Computed includes require preprocessing and are outside this source-level graph.
                continue;
            }
            if (internal) {
                const auto resolved =
                    (sourceRoot / el::Path{path}.parent() / target).resolveOrThrow(el::PathResolveMode::Lexical);
                if (!resolved.isRelativeTo(sourceRoot)) {
                    throw el::ApplicationError{
                        "Unresolved Quoted Include"_el, el::join("Include escapes the source tree: "_el, target)};
                }
                const auto relative = resolved.toRelativeOrThrow(sourceRoot).toPosix();
                if (!_nodes.contains(relative)) {
                    throw el::ApplicationError{
                        "Unresolved Quoted Include"_el, el::join("No inventoried source matches "_el, target)};
                }
                target = relative;
            }
            _edges.append(IncludeEdge{path, target, line, internal});
        }
    } catch (const el::ParseError &error) {
        const auto prefix = text.slice(el::CpRange{el::CpIndex{}, error.codePointIndex()});
        throw el::ApplicationError{"Invalid C++ Source"_el, "Source tokenization failed."_el, std::current_exception()}
            .withSourcePath((sourceRoot / path).toString())
            .withCodeLocation(
                el::CodeLocation{el::LineIndex::fromInteger(prefix.count("\n"_el).toRawValue()), el::ColumnIndex{}});
    } catch (const el::PathError &) {
        throw el::ApplicationError{
            "Invalid Include Path"_el, "The include target cannot be resolved."_el, std::current_exception()}
            .withSourcePath((sourceRoot / path).toString())
            .withCodeLocation(el::CodeLocation{line, el::ColumnIndex{}});
    } catch (const el::ApplicationError &error) {
        throw el::ApplicationError{
            el::ApplicationErrorContext{}
                .setTitle(error.title())
                .setDescription(error.description())
                .setSourcePath((sourceRoot / path).toString())
                .setCodeLocation(el::CodeLocation{line, el::ColumnIndex{}}),
            std::current_exception()};
    }
}

auto IncludeGraph::resolve(const el::String &identifier) const -> el::String {
    auto path = identifier.trimmed().replacedAll("\\"_el, "/"_el);
    if (path.startsWith("./"_el)) {
        path = path.slice(el::ByteRange::fromBegin(el::ByteIndex{2}));
    }
    if (path.startsWith("src/erbsland/"_el)) {
        path = path.slice(el::ByteRange::fromBegin(el::ByteIndex{13}));
    }
    if (_nodes.contains(path)) {
        return path;
    }
    auto matches = el::StringList{};
    const auto suffix = el::join("/"_el, path);
    for (const auto &candidate : _paths) {
        if (candidate.endsWith(suffix)) {
            matches.append(candidate);
        }
    }
    if (matches.count() == el::ItemCount{1}) {
        return matches.first();
    }
    throw el::ApplicationError{
        "Invalid Source Identifier"_el,
        el::join("Expected one source matching '"_el, identifier, "'. Candidates:\n"_el, matches.join("\n"_el))};
}

auto IncludeGraph::adjacent(const el::String &path, const bool reverse) const -> el::StringList {
    if (reverse) {
        return _dependents.get(path, {});
    }
    return _dependencies.get(path, {});
}

void IncludeGraph::buildIndexes() {
    auto external = el::StringHashSet{};
    for (const auto &edge : _edges) {
        _forward.set(edge.source, _forward.get(edge.source, {}).append(edge));
        if (edge.internal) {
            _reverse.set(edge.target, _reverse.get(edge.target, {}).append(edge));
            auto dependencies = _dependencies.get(edge.source, {});
            if (!dependencies.contains(edge.target)) {
                _dependencies.set(edge.source, dependencies.append(edge.target));
            }
            auto dependents = _dependents.get(edge.target, {});
            if (!dependents.contains(edge.source)) {
                _dependents.set(edge.target, dependents.append(edge.source));
            }
        } else {
            external.insert(edge.target);
        }
    }
    for (const auto &path : _paths) {
        _dependencies.set(path, _dependencies.get(path, {}).sort(less));
        _dependents.set(path, _dependents.get(path, {}).sort(less));
        auto forward = _forward.get(path, {});
        forward.sort([](const IncludeEdge &a, const IncludeEdge &b) -> bool {
            if (a.target != b.target) {
                return less(a.target, b.target);
            }
            return a.line < b.line;
        });
        _forward.set(path, std::move(forward));
    }
    for (const auto &name : external) {
        _external.append(name);
    }
    _external.sort(less);
    // Iterative Kosaraju: finish forward walks, then collect reverse components.
    auto visited = el::StringHashSet{};
    auto finished = el::StringList{};
    for (const auto &root : _paths) {
        if (!visited.tryInsert(root)) {
            continue;
        }
        auto pending = el::List<std::pair<el::String, el::ItemIndex>>{};
        pending.append({root, {}});
        while (!pending.isEmpty()) {
            auto [path, index] = pending.last();
            const auto neighbors = adjacent(path);
            if (index.isWithin(neighbors.count())) {
                pending.set(el::ItemIndex::last(pending.count()), {path, index + el::ItemCount{1}});
                const auto child = neighbors.get(index);
                if (visited.tryInsert(child)) {
                    pending.append({child, {}});
                }
            } else {
                pending.removeLast();
                finished.append(path);
            }
        }
    }
    visited.clear();
    while (!finished.isEmpty()) {
        const auto root = finished.last();
        finished.removeLast();
        if (!visited.tryInsert(root)) {
            continue;
        }
        auto component = el::StringList{root};
        for (auto index = el::ItemIndex{}; index.isWithin(component.count()); ++index) {
            for (const auto &child : adjacent(component.get(index), true)) {
                if (visited.tryInsert(child)) {
                    component.append(child);
                }
            }
        }
        component.sort(less);
        const auto componentIndex = el::ItemIndex::end(_components.count());
        for (const auto &path : component) {
            _componentIndex.set(path, componentIndex);
        }
        _components.append(std::move(component));
    }
}

auto IncludeGraph::reachable(const el::String &path, const bool reverse) const -> el::StringHashSet {
    auto visited = el::StringHashSet{};
    visited.insert(path);
    auto pending = el::StringList{path};
    for (auto index = el::ItemIndex{}; index.isWithin(pending.count()); ++index) {
        for (const auto &child : adjacent(pending.get(index), reverse)) {
            if (visited.tryInsert(child)) {
                pending.append(child);
            }
        }
    }
    visited.remove(path);
    return visited;
}

auto IncludeGraph::shortestPath(const el::String &source, const el::String &target) const -> el::StringList {
    auto previous = el::StringHashMap<el::String>{};
    previous.set(source, el::String{});
    auto pending = el::StringList{source};
    for (auto index = el::ItemIndex{}; index.isWithin(pending.count()); ++index) {
        const auto current = pending.get(index);
        if (current == target) {
            auto chain = el::StringList{};
            auto item = target;
            while (!item.isEmpty()) {
                chain.prepend(item);
                item = previous.get(item, {});
            }
            return chain;
        }
        for (const auto &child : adjacent(current)) {
            if (!previous.contains(child)) {
                previous.set(child, current);
                pending.append(child);
            }
        }
    }
    return {};
}

auto IncludeGraph::cycles() const -> el::List<el::StringList> {
    auto result = el::List<el::StringList>{};
    for (const auto &component : _components) {
        if (component.count() > el::ItemCount{1} || adjacent(component.first()).contains(component.first())) {
            result.append(component);
        }
    }
    result.sort([](const el::StringList &a, const el::StringList &b) -> bool { return less(a.first(), b.first()); });
    return result;
}

auto IncludeGraph::analyze(const el::String &target, const el::StringHashSet &excluded) const -> DependencyAnalysis {
    auto reached = reachable(target, true);
    reached.insert(target);
    auto result = DependencyAnalysis{};
    auto missing = el::StringHashSet{};
    for (const auto &path : _paths) {
        if (_nodes.get(path, {}).isPublic && !excluded.contains(path) && !reached.contains(path)) {
            missing.insert(path);
            result.missing.append(path);
        }
    }
    auto reachesMissing = el::List<bool>{}.resize(_components.count(), false);
    auto index = el::ItemIndex::end(_components.count());
    while (index > el::ItemIndex{}) {
        --index;
        const auto component = _components.get(index);
        auto representative = el::String{};
        auto downstream = false;
        for (const auto &path : component) {
            if (missing.contains(path) && representative.isEmpty()) {
                representative = path;
            }
            for (const auto &child : adjacent(path)) {
                const auto other = _componentIndex.get(child, {});
                if (other != index && reachesMissing.get(other)) {
                    downstream = true;
                }
            }
        }
        reachesMissing.set(index, downstream || !representative.isEmpty());
        if (!downstream && !representative.isEmpty()) {
            result.insertions.append(representative);
        }
    }
    result.insertions.sort(less);
    return result;
}

}
