// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../utilities/InternalView.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Char.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/CodeLocation.hpp"

#include <span>

namespace erbsland::conf::impl {

/// Represents a single decoded character from the line.
/// @tested{DecodedCharTest}
class DecodedChar final {
public:
    /// Create a new decoded character from a Core character.
    constexpr DecodedChar(
        const text::Char character, const unit::ByteIndex index, const unit::CodeLocation position) noexcept :
        _character{character}, _index{index}, _position{position} {}

    // defaults
    DecodedChar() = default;
    ~DecodedChar() = default;
    DecodedChar(const DecodedChar &) = default;
    DecodedChar(DecodedChar &&) = default;
    auto operator=(const DecodedChar &) -> DecodedChar & = default;
    auto operator=(DecodedChar &&) -> DecodedChar & = default;

public: // Accessors
    /// Access the decoded character.
    [[nodiscard]] constexpr auto character() const noexcept -> text::Char { return _character; }
    /// The start *byte*-index of this character in the current line.
    [[nodiscard]] constexpr auto index() const noexcept -> unit::ByteIndex { return _index; }

    /// The position of this character in the document.
    [[nodiscard]] constexpr auto codeLocation() const noexcept -> unit::CodeLocation { return _position; }

public: // testing
#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
    friend auto internalView(const DecodedChar &object) -> InternalViewPtr {
        auto result = InternalView::create();
        result->setValue(
            "unicode", text::StringFormat{"0x{:04x}"_el}.build(static_cast<uint32_t>(object._character.toRawValue())));
        result->setValue("index", text::StringFormat{"0x{:04x}"_el}.build(object._index.toSizeT()));
        result->setValue("location", object._position.toString());
        return result;
    }
#endif

private:
    text::Char _character{text::Char::endOfData()}; ///< The decoded character.
    unit::ByteIndex _index;                         ///< The start index of this character in the current line.
    unit::CodeLocation _position;                   ///< The position of this character in the document.
};

}
