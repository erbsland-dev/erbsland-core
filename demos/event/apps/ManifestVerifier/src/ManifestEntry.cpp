// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ManifestEntry.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/base_n/BaseNEncoder.hpp>
#include <erbsland/ByteInputStream.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringList.hpp>

#include <array>
#include <span>
#include <utility>

namespace demo {

using namespace el::text::literals;

ManifestEntry::ManifestEntry(el::String digest, el::Path name) : _digest{std::move(digest)}, _name{std::move(name)} {
}

auto ManifestEntry::read(const el::Path &path) -> el::List<ManifestEntry> {
    static const auto record = el::RegEx::compile("([A-Fa-f0-9]{64})  (.+)"_el);
    const auto lines = el::StringList::fromSplit(
        path.content().readTextOrThrow(), el::CharSet{U'\n'}, el::ItemCount::infinite(), true);
    auto entries = el::List<ManifestEntry>{};
    auto number = el::ItemCount{};
    for (const auto &line : lines) {
        ++number;
        const auto text = line.trimmed(el::CharSet{U'\r'}, el::StringSide::Back);
        if (text.isEmpty()) {
            continue;
        }
        const auto match = record->fullMatch(text);
        if (!match) {
            throw el::ApplicationError{
                el::StringFormat{"Expected SHA-256, two spaces and a file name on line {}."_el}.build(number)};
        }
        auto name = el::Path::fromNativeOrThrow(match->content(2));
        if (!name.isRelative()) {
            throw el::ApplicationError{el::StringFormat{"Expected a relative file name on line {}."_el}.build(number)};
        }
        entries.append(ManifestEntry{match->content(1), std::move(name)});
    }
    return entries;
}

auto ManifestEntry::matchesFile(const el::Path &directory) const -> bool {
    const auto path = directory / _name;
    if (!path.info().isRegularFile()) {
        throw el::ApplicationError{el::StringFormat{"Expected a regular file: {}"_el}.build(path)};
    }
    const auto input = path.content().openByteInputStream();
    auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
    auto buffer = std::array<el::Byte, 64 * 1024>{};
    while (true) {
        const auto read = input->read(buffer);
        if (read.isFinished()) {
            break;
        }
        if (read.isTimeout()) {
            throw el::ApplicationError{"Timed out while hashing a file."_el};
        }
        hasher.update(std::span{buffer}.first(read.data().toSizeT()));
    }
    const auto actual = el::base_n::BaseNEncoder{hasher.finalize(), el::base_n::BaseNFormat::base16()}.toString();
    return actual.compare(_digest, el::Char::compareCaseFolded) == 0;
}

}
