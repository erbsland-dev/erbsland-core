// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/u16/U16StringView.hpp"
#include "../../../unit/U16DataIndex.hpp"
#include "../../Input16.hpp"

namespace erbsland::re::impl {

/// An input that reads from an owning UTF-16 string view.
class U16StringInput final : public Input16 {
public:
    [[nodiscard]] static auto create(const text::U16StringView &text) noexcept -> Input16Ptr {
        return std::make_shared<U16StringInput>(text);
    }
    explicit U16StringInput(const text::U16StringView &text) noexcept : _text{text} {}

public: // implement InputBase
    [[nodiscard]] auto read() -> CharAndPosition override;
    [[nodiscard]] auto peek() -> CharAndPosition override;
    void skip(unit::CpLength characterCount) override;

public: // implement Input16
    [[nodiscard]] auto createMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList) -> Match16Ptr override;

private:
    text::U16StringView _text;
    unit::U16DataIndex _position;
};

}
