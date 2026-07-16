// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/u32/U32StringView.hpp"
#include "../../../unit/CpIndex.hpp"
#include "../../Input32.hpp"

namespace erbsland::re::impl {

/// An input that reads from an owning UTF-32 string view.
class U32StringInput final : public Input32 {
public:
    [[nodiscard]] static auto create(const text::U32StringView &text) noexcept -> Input32Ptr {
        return std::make_shared<U32StringInput>(text);
    }
    explicit U32StringInput(const text::U32StringView &text) noexcept : _text{text} {}

public: // implement InputBase
    [[nodiscard]] auto read() -> CharAndPosition override;
    [[nodiscard]] auto peek() -> CharAndPosition override;
    void skip(unit::CpLength characterCount) override;

public: // implement Input32
    [[nodiscard]] auto createMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList) -> Match32Ptr override;

private:
    text::U32StringView _text;
    unit::CpIndex _position;
};

}
