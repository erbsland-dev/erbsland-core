// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TagCatalog.hpp"

#include "WorkflowFile.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/StringTransformations.hpp>

namespace tool::github_workflows {

using namespace el::text::literals;

TagCatalog::TagCatalog(const el::String &text, const el::String &repository) {
    auto objects = el::StringHashMap<el::String>{};
    auto peeled = el::StringHashMap<el::String>{};
    auto lines = el::StringSplitter{text, U'\n'};
    while (!lines.isAtEnd()) {
        const auto line = lines.next().trimmed(el::CharSet{U'\r'}, el::StringSide::Back);
        if (line.isEmpty()) {
            continue;
        }
        const auto pair = line.splitAtFirst("\t"_el);
        if (!pair || !WorkflowFile::isSha(pair->first) || !pair->second.startsWith("refs/tags/"_el) ||
            pair->second.containsOneOf(el::CharSet{U' ', U'\t'})) {
            throw el::ApplicationError{
                "Invalid Git Tag Response"_el,
                el::join(
                    "Malformed tag record for "_el,
                    repository,
                    ": "_el,
                    line.slice(el::StringSide::Front, el::CpCount{160}))};
        }
        auto name = pair->second.slice(el::ByteRange::fromBegin(el::ByteIndex{10}));
        const auto isPeeled = name.endsWith("^{}"_el);
        if (isPeeled) {
            name = name.slice(el::ByteRange{el::ByteIndex{}, name.length() - el::ByteCount{3}});
        }
        if (!StableTag::fromString(name)) {
            continue;
        }
        const auto insert = [&](el::StringHashMap<el::String> &map) -> void {
            const auto sha = el::toAsciiLowercase(pair->first);
            if (map.contains(name) && map.get(name, {}) != sha) {
                throw el::ApplicationError{"Conflicting Git Tags"_el, el::join(repository, ": "_el, name)};
            }
            map.set(name, sha);
        };
        if (isPeeled) {
            insert(peeled);
        } else {
            insert(objects);
        }
    }
    for (const auto &[name, sha] : peeled) {
        if (!objects.contains(name)) {
            throw el::ApplicationError{
                "Invalid Git Tag Response"_el,
                el::join("Peeled tag has no tag object in "_el, repository, ": "_el, name)};
        }
    }
    for (const auto &[name, sha] : objects) {
        _tags.set(name, ActionTag{*StableTag::fromString(name), peeled.get(name, sha)});
    }
}

auto TagCatalog::higher(const ActionTag &left, const ActionTag &right) -> bool {
    if (left.version != right.version) {
        return left.version > right.version;
    }
    return left.version.toString().compareBinary(right.version.toString()) > 0;
}

auto TagCatalog::named(const el::String &name) const -> std::optional<ActionTag> {
    if (_tags.contains(name)) {
        return _tags.get(name, {});
    }
    return {};
}

auto TagCatalog::latest() const -> std::optional<ActionTag> {
    auto result = std::optional<ActionTag>{};
    for (const auto &[name, tag] : _tags) {
        if (!result || higher(tag, *result)) {
            result = tag;
        }
    }
    return result;
}

auto TagCatalog::forSha(const el::String &sha) const -> std::optional<ActionTag> {
    const auto normalized = el::toAsciiLowercase(sha);
    auto result = std::optional<ActionTag>{};
    for (const auto &[name, tag] : _tags) {
        if (tag.sha == normalized && (!result || higher(tag, *result))) {
            result = tag;
        }
    }
    return result;
}

}
