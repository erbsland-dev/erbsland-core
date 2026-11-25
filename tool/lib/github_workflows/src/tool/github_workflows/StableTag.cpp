// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StableTag.hpp"

#include <erbsland/OverflowError.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/RegEx.hpp>

namespace tool::github_workflows {

using namespace el::text::literals;

auto StableTag::fromString(const el::String &text) -> std::optional<StableTag> {
    static const auto cPattern =
        el::RegEx::compile(R"(v(?<major>0|[1-9][0-9]*)\.(?<minor>0|[1-9][0-9]*)\.(?<patch>0|[1-9][0-9]*))"_el);
    const auto match = cPattern->fullMatch(text);
    if (!match) {
        return {};
    }
    auto tag = StableTag{};
    tag._text = text;
    try {
        tag._parts = {
            match->content("major"_el).toIntegerOrThrow<uint64_t>(),
            match->content("minor"_el).toIntegerOrThrow<uint64_t>(),
            match->content("patch"_el).toIntegerOrThrow<uint64_t>()};
    } catch (const el::OverflowError &) {
        return {};
    }
    return tag;
}

}
