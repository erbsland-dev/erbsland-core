// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../StringView.hpp"

#include <utility>

namespace erbsland::text::html::impl {

/// One parsed HTML attribute.
/// @tested{HtmlParserTest}
class HtmlAttribute final {
public:
    /// Create one parsed attribute.
    /// @param name The lower-case ASCII attribute name.
    /// @param value The attribute value.
    HtmlAttribute(StringView name = {}, StringView value = {}) noexcept :
        _name{std::move(name)}, _value{std::move(value)} {}

public:
    /// Access the lower-case ASCII attribute name.
    [[nodiscard]] auto name() const noexcept -> const StringView & { return _name; }
    /// Replace the attribute name.
    /// @param name The new attribute name.
    void setName(StringView name) noexcept { _name = std::move(name); }
    /// Access the attribute value.
    [[nodiscard]] auto value() const noexcept -> const StringView & { return _value; }
    /// Replace the attribute value.
    /// @param value The new attribute value.
    void setValue(StringView value) noexcept { _value = std::move(value); }

private:
    StringView _name;  ///< The lower-case ASCII attribute name.
    StringView _value; ///< The attribute value.
};

}
