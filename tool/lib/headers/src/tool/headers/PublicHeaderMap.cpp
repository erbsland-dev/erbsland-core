// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicHeaderMap.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CodeLocation.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringHashSet.hpp>
#include <erbsland/StringSplitter.hpp>

namespace tool::headers {

using namespace el::text::literals;

void PublicHeaderMap::read(const el::String &text, const el::String &source) {
    auto parsed = PublicHeaderMap{};
    auto previous = el::String{};
    auto pairs = el::StringHashSet{};
    static const auto cPath = el::RegEx::compile(R"/([A-Za-z0-9_.\-]+(?:/[A-Za-z0-9_.\-]+)*)/"_el);
    auto lines = el::StringSplitter{text, U'\n'};
    auto lineIndex = el::LineIndex{};
    while (!lines.isAtEnd()) {
        const auto line = lines.next().trimmed(el::CharSet{U'\r'}, el::StringSide::Back);
        const auto location = lineIndex++;
        if (line.trimmed().isEmpty() || line.startsWith("#"_el)) {
            continue;
        }
        const auto fail = [&](const el::String &description) -> void {
            throw el::ApplicationError{el::ApplicationErrorContext{}
                    .setTitle("Invalid Public Header Mapping"_el)
                    .setDescription(description)
                    .setSourcePath(source)
                    .setCodeLocation(el::CodeLocation{location, el::ColumnIndex{}})};
        };
        const auto parts = el::StringList::fromSplit(line, el::CharSet{U';'}, el::ItemCount::infinite(), true);
        if (parts.count() != el::ItemCount{2}) {
            fail("Expected a source path and public include separated by one semicolon."_el);
        }
        const auto &header = parts.get(el::ItemIndex{0});
        const auto &include = parts.get(el::ItemIndex{1});
        for (const auto &path : parts) {
            if (!cPath->fullMatch(path) || el::Path{path}.elements().contains("."_el) ||
                el::Path{path}.elements().contains(".."_el)) {
                fail("Mapping paths must be portable relative paths without dot components."_el);
            }
        }
        if (header.compareBinary(previous) < 0 || !pairs.tryInsert(line)) {
            fail("Mapping records must be sorted by source path and must not repeat a pair."_el);
        }
        previous = header;
        if (!parsed._canonical.contains(header)) {
            parsed._canonical.set(header, include);
        }
        parsed._sources.set(include, parsed._sources.get(include, {}).append(header));
    }
    if (parsed._canonical.count().isZero()) {
        throw el::ApplicationError{"Empty Public Header Mapping"_el, "Run update-public-headers first."_el}
            .withSourcePath(source);
    }
    *this = std::move(parsed);
}

}
