// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HtmlAttribute.hpp"
#include "HtmlTokenType.hpp"

#include "../../String.hpp"

#include <utility>
#include <vector>

namespace erbsland::text::html::impl {

/// One token produced by the HTML tokenizer.
/// @tested{HtmlTokenizerTest}
struct HtmlToken final {
    using Attributes = std::vector<HtmlAttribute>;

    /// Create a token.
    /// @param type The token type.
    /// @param value The token value.
    /// @param attributes The attributes for an opening tag.
    /// @param selfClosing Whether this opening tag uses self-closing syntax.
    HtmlToken(
        HtmlTokenType type = HtmlTokenType::End,
        String value = {},
        Attributes attributes = {},
        bool selfClosing = false) :
        type{type}, value{std::move(value)}, attributes{std::move(attributes)}, selfClosing{selfClosing} {}

    HtmlTokenType type{HtmlTokenType::End}; ///< The token type.
    String value;                           ///< The token value.
    Attributes attributes;                  ///< The attributes for an opening tag.
    bool selfClosing{false};                ///< Whether this opening tag uses self-closing syntax.
};

/// Swap two tokens without copying their payload.
inline void swap(HtmlToken &left, HtmlToken &right) noexcept {
    using std::swap;

    swap(left.type, right.type);
    swap(left.value, right.value);
    swap(left.attributes, right.attributes);
    swap(left.selfClosing, right.selfClosing);
}

}
