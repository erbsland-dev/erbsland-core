// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "XmlAttribute.hpp"
#include "XmlNode_fwd.hpp"
#include "XmlNodeType.hpp"

#include "impl/XmlCodec_fwd.hpp"

#include "../../text/AnyStringBuilder.hpp"
#include "../../text/String.hpp"
#include "../../util/List.hpp"

#include <optional>

namespace erbsland::data::xml {
/// A mutable XML DOM node with ordered children.
/// @seedoc{/reference/data/xml}
/// @tested{XmlDocumentTest}
class XmlNode final {
    friend class impl::XmlCodec;

private:
    /// Construct one DOM node.
    /// @param type The node kind.
    /// @param name The element, instruction, or reference name.
    /// @param text The node text or preserved markup.
    XmlNode(XmlNodeType type, text::String name, text::String text) :
        _type{type}, _name{std::move(name)}, _text{std::move(text)} {}

public: // accessors
    /// Get the node kind.
    [[nodiscard]] auto type() const noexcept -> XmlNodeType { return _type; }
    /// Get an element, instruction, or entity name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }
    /// Get this node's own text or preserved markup.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get ordered attributes.
    [[nodiscard]] auto attributes() const noexcept -> const util::List<XmlAttribute> & { return _attributes; }
    /// Get ordered children.
    [[nodiscard]] auto children() const noexcept -> const util::List<XmlNodePtr> & { return _children; }
    /// Get a decoded attribute, or no value if absent or unresolved.
    [[nodiscard]] auto getAttribute(const text::String &name) const -> std::optional<text::String>;
    /// Get all descendant character data, or no value if an entity is unresolved.
    [[nodiscard]] auto textContent() const -> std::optional<text::String>;
    /// Get all descendant character data.
    /// @throws err::LogicError If an entity is unresolved.
    [[nodiscard]] auto textContentOrThrow() const -> text::String;

public: // mutation
    /// Set or replace a plain-text attribute.
    auto setAttribute(text::String name, text::String value) -> XmlNode &;
    /// Append a child node to an element.
    auto add(XmlNodePtr child) -> XmlNodePtr;
    /// Create and append an element with optional text in one step.
    auto addElement(text::String name, text::String text = {}) -> XmlNodePtr;
    /// Create and append text.
    auto addText(text::String text) -> XmlNodePtr;

public: // factories
    /// Create an element, optionally with a text child.
    [[nodiscard]] static auto createElement(text::String name, text::String text = {}) -> XmlNodePtr;
    /// Create a text node.
    [[nodiscard]] static auto createText(text::String text) -> XmlNodePtr;
    /// Create a CDATA node.
    [[nodiscard]] static auto createCData(text::String text) -> XmlNodePtr;
    /// Create a comment node.
    [[nodiscard]] static auto createComment(text::String text) -> XmlNodePtr;
    /// Create a processing instruction.
    [[nodiscard]] static auto createProcessingInstruction(text::String name, text::String text) -> XmlNodePtr;
    /// Create an unresolved entity reference; `name` omits `&` and `;`.
    [[nodiscard]] static auto createEntityReference(text::String name) -> XmlNodePtr;
    /// Create a preserved DOCTYPE declaration including its markup.
    [[nodiscard]] static auto createDocType(text::String raw) -> XmlNodePtr;
    /// Create a preserved XML declaration including its markup.
    [[nodiscard]] static auto createDeclaration(text::String raw) -> XmlNodePtr;

private: // reference resolution
    /// Resolve a predefined or numeric entity reference.
    /// @param name The reference name without delimiters.
    /// @return Decoded text, or no value for an unresolved reference.
    [[nodiscard]] static auto resolveReference(const text::String &name) -> std::optional<text::String>;
    /// Resolve references preserved inside an attribute value.
    /// @param attribute The attribute to inspect.
    /// @return Decoded text, or no value if a reference is unresolved.
    [[nodiscard]] static auto resolveAttribute(const XmlAttribute &attribute) -> std::optional<text::String>;
    /// Append all resolvable descendant character data.
    /// @param builder The output builder.
    /// @return False if an unresolved reference was encountered.
    auto appendTextContent(text::AnyStringBuilder &builder) const -> bool;

private:                                  // data
    XmlNodeType _type;                    ///< Node kind.
    text::String _name;                   ///< Name for named nodes.
    text::String _text;                   ///< Content or preserved markup.
    util::List<XmlAttribute> _attributes; ///< Ordered attributes.
    util::List<XmlNodePtr> _children;     ///< Ordered descendants.
};
}
