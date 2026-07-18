// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../String.hpp"

#include <utility>

namespace erbsland::text::html::impl {

/// One parsed HTML attribute.
/// @tested{HtmlParserTest}
class HtmlAttribute final {
public:
    /// Create one parsed attribute.
    /// @param name The lower-case ASCII attribute name.
    /// @param value The attribute value.
    HtmlAttribute(String name = {}, String value = {}) noexcept : _name{std::move(name)}, _value{std::move(value)} {}

public:
    /// Access the lower-case ASCII attribute name.
    [[nodiscard]] auto name() const noexcept -> const String & { return _name; }
    /// Replace the attribute name.
    /// @param name The new attribute name.
    void setName(String name) noexcept { _name = std::move(name); }
    /// Access the attribute value.
    [[nodiscard]] auto value() const noexcept -> const String & { return _value; }
    /// Replace the attribute value.
    /// @param value The new attribute value.
    void setValue(String value) noexcept { _value = std::move(value); }

private:
    String _name;  ///< The lower-case ASCII attribute name.
    String _value; ///< The attribute value.
};

}
