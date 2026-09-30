// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/XmlCodec_fwd.hpp"

#include "../../text/String.hpp"

#include <optional>

namespace erbsland::data::xml {
/// An ordered XML attribute with preserved references from parsed input.
/// @tested{XmlDocumentTest}
class XmlAttribute final {
    friend class impl::XmlCodec;

public: // constructor
    /// Create a plain-text attribute.
    XmlAttribute(text::String name, text::String value) : _name{std::move(name)}, _value{std::move(value)} {}

public: // accessors
    /// Get the qualified attribute name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Get the stored value; parsed references retain their original spelling.
    [[nodiscard]] auto value() const noexcept -> const text::String & { return _value; }
    /// Test whether the stored value contains preserved references.
    [[nodiscard]] auto hasReferences() const noexcept -> bool { return _hasReferences; }

private:                        // data
    text::String _name;         ///< Qualified name.
    text::String _value;        ///< Plain value or parsed value with references.
    bool _hasReferences{false}; ///< Parsed references are preserved.
};
}
