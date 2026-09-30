// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../XmlDocument.hpp"

#include "../../../text/AnyStringBuilder.hpp"
#include "../../../text/StringCharReader.hpp"

namespace erbsland::data::xml::impl {
/// Internal XML parser and serializer.
/// @tested{XmlDocumentTest}
class XmlCodec final {
public: // conversion
    /// Parse one complete XML document with bounded input.
    /// @param source The XML source text.
    /// @param options The parse or format options.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto decode(const text::String &source, XmlParseOptions options) -> XmlDocument;
    /// Serialize an XML DOM document.
    /// @param document The document to serialize.
    /// @param options The parse or format options.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto encode(const XmlDocument &document, XmlFormatOptions options) -> text::String;

private: // construction
    /// Create a codec for parsing or formatting.
    /// @param source The XML source text.
    /// @param options The parse or format options.
    explicit XmlCodec(text::String source, XmlParseOptions options);
    /// Create a codec for parsing or formatting.
    /// @param options The parse or format options.
    explicit XmlCodec([[maybe_unused]] XmlFormatOptions options);

private: // parsing
    /// Parse prolog, root element, and epilog.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto parse() -> XmlDocument;
    /// Read one element and its ordered children.
    /// @param depth The current container nesting depth.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readElement(unit::ItemCount depth) -> XmlNodePtr;
    /// Read a comment body.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readComment() -> XmlNodePtr;
    /// Read a CDATA section.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readCData() -> XmlNodePtr;
    /// Read a processing instruction or declaration.
    /// @param declaration Whether this is an XML declaration.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readInstruction(bool declaration) -> XmlNodePtr;
    /// Preserve a DOCTYPE and its internal subset.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readDocType() -> XmlNodePtr;
    /// Read an ordinary character-data node.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readText() -> XmlNodePtr;
    /// Read and preserve an entity reference.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readReference() -> XmlNodePtr;
    /// Read an XML qualified name.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readName() -> text::String;
    /// Read text up to a terminating token.
    /// @param terminator The token ending this text.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] auto readUntil(const text::String &terminator) -> text::String;
    /// Read and append one XML attribute.
    /// @param node The node being read or serialized.
    void readAttribute(XmlNode &node);
    /// Skip XML whitespace at the current position.
    void skipWhitespace();
    /// Enforce the configured DOM node count limit.
    void countNode();

private: // serialization
    /// Serialize one DOM node recursively.
    /// @param node The node being read or serialized.
    void writeNode(const XmlNode &node);
    /// Escape text or an attribute value.
    /// @param value The value to read, write, or inspect.
    /// @param attribute Whether the content is an attribute value.
    /// @param preserveReferences Whether references are already encoded.
    void writeEscaped(const text::String &value, bool attribute, bool preserveReferences = false);
    /// Reject XML 1.0 forbidden character code points before serialization.
    /// @param value The text to validate.
    /// @throws err::ParameterError If the text contains a forbidden code point.
    static void validateXmlText(const text::String &value);
    /// Check XML 1.0 name syntax.
    /// @param value The value to read, write, or inspect.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto validName(const text::String &value) -> bool;
    /// Check one XML 1.0 name code point.
    /// @param character The Unicode code point to check.
    /// @param first Whether this is the first name code point.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto validNameChar(text::Char character, bool first) noexcept -> bool;
    /// Check one XML 1.0 character code point.
    /// @param character The Unicode code point to check.
    /// @return The decoded, encoded, or checked result.
    [[nodiscard]] static auto validXmlChar(text::Char character) noexcept -> bool;
    /// Report malformed or unsupported XML input.
    /// @throws err::ParseError Always, for malformed or unsupported input.
    [[noreturn]] static void fail();

private:                             // data
    text::String _source;            ///< Shared parser source.
    text::StringCharReader _reader;  ///< Sole XML reader.
    text::AnyStringBuilder _builder; ///< Sole XML writer.
    XmlParseOptions _parseOptions;   ///< Parse limits.
    unit::ItemCount _nodeCount;      ///< Parsed nodes.
};
}
