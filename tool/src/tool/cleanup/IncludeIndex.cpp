// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IncludeIndex.hpp"

#include "CleanupText.hpp"

#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

#include <algorithm>
#include <optional>

namespace tool::cleanup {

using namespace text;
using namespace el::text::literals;

IncludeIndex::IncludeIndex(const CleanupPolicy &policy, const el::Path &projectRoot) :
    _library{policy.library}, _publicPrefix{el::join(policy.publicIncludePrefix.toString(), "/"_el)} {
    const auto relative = _library.toRelativeOrThrow(projectRoot);
    if (relative != el::Path::currentElement()) {
        _sourcePrefix = el::join(relative.toString(), "/"_el);
        _sourceMarker = el::join("/"_el, _sourcePrefix);
    }
}

void IncludeIndex::add(const el::Path &path) {
    _files.insert(path.toString());
    if (path.isRelativeTo(_library)) {
        const auto name = path.toRelativeOrThrow(_library).toString();
        _libraryFiles.insert(name);
        auto candidates = _byName.get(path.name(), {});
        if (!candidates.contains(name)) {
            candidates.append(name);
            _byName.set(path.name(), std::move(candidates));
        }
    }
}

void IncludeIndex::readMapping(const el::String &text) {
    _mapping.read(text);
}

auto IncludeIndex::contains(const el::Path &path) const -> bool {
    return containsNormalized(path.resolveOrThrow(el::PathResolveMode::Lexical));
}

auto IncludeIndex::containsNormalized(const el::Path &path) const -> bool {
    return _files.contains(path.toString());
}

auto IncludeIndex::local(const el::Path &file, const el::String &name, const SourceTree &tree) const -> bool {
    const auto relative = (file.parent() / name).resolveOrThrow(el::PathResolveMode::Lexical);
    const auto inLocalRoot = relative.isRelativeTo(tree.directory) ||
        std::any_of(tree.includeRoots.begin(), tree.includeRoots.end(), [&](const el::Path &root) -> bool {
            return relative.isRelativeTo(root);
        });
    if (inLocalRoot &&
        (containsNormalized(relative) || containsNormalized(relative.withName(el::join(relative.name(), ".in"_el))))) {
        return true;
    }
    const auto path = el::Path{name};
    if (!path.isRelative() || path.elements().contains(".."_el)) {
        return false;
    }
    for (const auto &root : tree.includeRoots) {
        const auto candidate = (root / path).resolveOrThrow(el::PathResolveMode::Lexical);
        if (containsNormalized(candidate) ||
            containsNormalized(candidate.withName(el::join(candidate.name(), ".in"_el)))) {
            return true;
        }
    }
    return false;
}

auto IncludeIndex::publicName(const el::String &source) const -> el::String {
    return el::join(_publicPrefix, _mapping.canonical().get(source, source));
}

auto IncludeIndex::libraryTarget(const el::String &name, const el::Path &file) const -> el::String {
    if (_libraryFiles.contains(name)) {
        return name;
    }
    const auto candidates = _byName.get(el::Path{name}.name(), {});
    auto suffixes = el::StringList{};
    const auto suffix = el::join("/"_el, name);
    for (const auto &candidate : candidates) {
        if (candidate.endsWith(suffix)) {
            suffixes.append(candidate);
        }
    }
    if (suffixes.count() == el::ItemCount{1}) {
        return suffixes.get(el::ItemIndex{0});
    }
    if (suffixes.isEmpty() && candidates.count() == el::ItemCount{1}) {
        return candidates.get(el::ItemIndex{0});
    }
    if (suffixes.isEmpty() && file.isRelativeTo(_library)) {
        const auto context = file.parent().toRelativeOrThrow(_library).elements();
        auto best = el::String{};
        auto bestScore = el::ItemIndex::zero();
        for (const auto &candidate : candidates) {
            const auto elements = el::Path{candidate}.parent().elements();
            auto score = el::ItemIndex::zero();
            while (
                score.isWithin(context.count()) && score.isWithin(elements.count()) &&
                context.get(score) == elements.get(score)) {
                ++score;
            }
            if (score > bestScore) {
                bestScore = score;
                best = candidate;
            } else if (score == bestScore) {
                best = {};
            }
        }
        if (!best.isEmpty()) {
            return best;
        }
    }
    if (candidates.isEmpty()) {
        fail(
            el::StringFormat{"Cannot resolve include '{}': no matching library or local file."_el}.build(name),
            file.toString());
    }
    fail(
        el::StringFormat{"Ambiguous include '{}'. Candidates:\n{}"_el}.build(name, candidates.sorted().join("\n"_el)),
        file.toString());
}

auto IncludeIndex::resolve(const el::Path &file, const el::String &name, bool &global, const SourceTree &tree) const
    -> el::String {
    const auto includePath = el::Path{name};
    if (!includePath.isRelative()) {
        fail("Absolute include paths are not supported."_el, file.toString());
    }
    const auto relative = (file.parent() / name).resolveOrThrow(el::PathResolveMode::Lexical);
    if (relative.isRelativeTo(_library) && containsNormalized(relative)) {
        const auto source = relative.toRelativeOrThrow(_library).toString();
        if (tree.publicIncludes) {
            global = true;
            return publicName(source);
        }
        global = false;
        return relative.toRelativeOrThrow(file.parent()).toString();
    }
    if (tree.publicIncludes && local(file, name, tree)) {
        return name;
    }
    auto candidate = name;
    auto sourceSuffix = std::optional<el::String>{};
    if (!_sourcePrefix.isEmpty()) {
        if (candidate.startsWith(_sourcePrefix)) {
            sourceSuffix = candidate.slice(el::StringSide::Back, el::ByteIndex::end(_sourcePrefix.length()));
        } else {
            // Only match complete path components, never a suffix such as 'other-src/library'.
            if (const auto parts = candidate.splitAtFirst(_sourceMarker)) {
                sourceSuffix = parts->second;
            }
        }
    }
    if (sourceSuffix) {
        candidate = *sourceSuffix;
    } else if (candidate.startsWith(_publicPrefix)) {
        candidate = candidate.slice(el::StringSide::Back, el::ByteIndex::end(_publicPrefix.length()));
        if (tree.publicIncludes) {
            global = true;
            const auto sources = _mapping.sources().get(candidate, {});
            if (sources.count() == el::ItemCount{1}) {
                return publicName(sources.get(el::ItemIndex{0}));
            }
            if (_mapping.canonical().contains(candidate)) {
                return publicName(candidate);
            }
            return name; // Merged umbrellas, internal headers, and other Erbsland libraries.
        }
    } else if (
        global && (!tree.publicIncludes || candidate.contains("/"_el) || !_byName.contains(includePath.name()))) {
        return name;
    }
    const auto target = libraryTarget(candidate, file);
    if (tree.publicIncludes) {
        global = true;
        return publicName(target);
    }
    global = false;
    return (_library / target).toRelativeOrThrow(file.parent()).toString();
}

}
