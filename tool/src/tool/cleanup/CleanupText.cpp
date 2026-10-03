// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CleanupText.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/Hasher.hpp>
#include <erbsland/StringJoins.hpp>

namespace tool::cleanup::text {

using namespace el::text::literals;

auto lines(const el::String &text) -> el::StringList {
    if (text.isEmpty()) {
        return {};
    }
    auto result = el::StringList::fromSplit(text, el::CharSet{U'\n'}, el::ItemCount::infinite(), true);
    result.map([](const el::String &line) { return line.trimmed(el::CharSet{U'\r'}, el::StringSide::Back); });
    if (result.last().isEmpty() && text.endsWith("\n"_el)) {
        result.removeLast();
    }
    return result;
}

auto join(const el::StringList &lines) -> el::String {
    if (lines.isEmpty()) {
        return {};
    }
    return el::join(lines.join("\n"_el), "\n"_el);
}

auto digest(const el::String &text) -> el::String {
    auto hasher = el::Hasher{el::HashAlgorithm::Sha2_256};
    hasher.update(text);
    return el::String::fromByteBlock(hasher.finalize());
}

void fail(const el::String &description, const el::String &path) {
    throw el::ApplicationError{"Code Cleanup Failed"_el, description}.withSourcePath(path);
}

}
