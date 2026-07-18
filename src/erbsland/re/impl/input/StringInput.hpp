// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../Input.hpp"

namespace erbsland::re::impl {

/// An input that reads from an owning UTF-8 read-only string.
class StringInput final : public Input {
public:
    [[nodiscard]] static auto create(const text::String &text) noexcept -> InputPtr {
        return std::make_shared<StringInput>(text);
    }
    explicit StringInput(const text::String &text) noexcept : _text{text} {}

public: // implement Input
    [[nodiscard]] auto read() -> CharAndPosition override;
    [[nodiscard]] auto peek() -> CharAndPosition override;
    void skip(unit::CpLength characterCount) override;
    [[nodiscard]] auto createMatch(CaptureGroupList captureGroupList) -> MatchPtr override;

private:
    text::String _text;
    unit::ByteIndex _position;
};

}
