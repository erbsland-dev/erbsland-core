// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "XmlFormatOptions.hpp"
#include "XmlNode.hpp"
#include "XmlParseOptions.hpp"

#include "../../text/String.hpp"
#include "../../util/List.hpp"

#include <optional>

namespace erbsland::data::xml {
/// A mutable XML document with ordered prolog and root nodes.
/// @seedoc{/reference/data/xml}
/// @tested{XmlDocumentTest}
class XmlDocument final {
public:
    // defaults
    /// Create an empty document.
    XmlDocument() = default;

public: // accessors
    /// Get all top-level nodes in document order.
    [[nodiscard]] auto nodes() const noexcept -> const util::List<XmlNodePtr> & { return _nodes; }
    /// Get the document element, or an empty pointer.
    [[nodiscard]] auto root() const noexcept -> XmlNodePtr;

public: // mutation
    /// Add a top-level node.
    auto add(XmlNodePtr node) -> XmlNodePtr;
    /// Create and add the document element with optional text.
    auto addRoot(text::String name, text::String text = {}) -> XmlNodePtr;

public: // conversion
    /// Serialize this document as XML text.
    [[nodiscard]] auto toString(XmlFormatOptions options = {}) const -> text::String;
    /// Parse XML, returning no value on failure.
    [[nodiscard]] static auto fromString(const text::String &text, XmlParseOptions options = {}) noexcept
        -> std::optional<XmlDocument>;
    /// Parse an XML document.
    /// @throws err::ParseError For malformed or unsupported input.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text, XmlParseOptions options = {}) -> XmlDocument;

private:                           // data
    util::List<XmlNodePtr> _nodes; ///< Prolog, document element, and epilog nodes.
};
}
