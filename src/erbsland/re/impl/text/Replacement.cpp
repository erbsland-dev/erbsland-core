// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Replacement.hpp"

#include "ReplacementParser.hpp"

namespace erbsland::re::impl {

void Replacement::appendTo(text::StringEditor &text, const MatchPtr &match) const {
    const auto replacementLength = length(match);
    if (replacementLength.isZero()) {
        return;
    }
    text.reserve(text.length() + replacementLength);
    for (const auto &part : _parts) {
        std::visit(
            [&]<typename T>(const T &value) -> void {
                if constexpr (std::is_same_v<std::decay_t<T>, StaticText>) {
                    text.append(value.text);
                } else if constexpr (std::is_same_v<std::decay_t<T>, CaptureGroup>) {
                    text.append(match->content(value.index));
                }
            },
            part);
    }
}

auto Replacement::length(const MatchPtr &match) const -> unit::ByteLength {
    auto result = unit::ByteLength{};
    for (const auto &part : _parts) {
        std::visit(
            [&]<typename T>(const T &value) -> void {
                if constexpr (std::is_same_v<std::decay_t<T>, StaticText>) {
                    result += value.text.length();
                } else if constexpr (std::is_same_v<std::decay_t<T>, CaptureGroup>) {
                    result += unit::ByteLength::fromSizeT(match->range(value.index).size());
                }
            },
            part);
    }
    return result;
}

auto Replacement::create(const text::String &expression, const CaptureGroupNames &groupNames) -> Replacement {
    ReplacementParser parser{expression, groupNames};
    return parser.parse();
}

}
