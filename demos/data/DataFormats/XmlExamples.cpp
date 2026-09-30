// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "DataFormatsDemos.hpp"

#include <DemoCommon.hpp>
#include <erbsland/data/xml/XmlDocument.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParseError.hpp>

namespace demo {

using namespace el::text::literals;
using el::xml::XmlDocument;
using el::xml::XmlNode;
using el::xml::XmlNodeType;
using el::xml::XmlParseOptions;

/// Parse a complete XML document and report a malformed one.
///
/// `fromString()` returns an optional document for validation workflows. `fromStringOrThrow()` reports a `ParseError`
/// when the caller needs a diagnostic. Both expect exactly one root element.
void parseXmlDocument() {
    // Accept a complete document and read a value from its first child element.
    const auto source = "<orkester><gruppe navn=\"strygere\"><instrument>violin</instrument></gruppe></orkester>"_el;
    if (const auto document = XmlDocument::fromString(source)) {
        el::io::printLine("Root: "_el, document->root()->name());
        el::io::printLine(
            "Group: "_el, document->root()->children().get(el::ItemIndex{0U})->getAttribute("navn"_el).value());
    }
    // Use the throwing form when the caller needs a parse diagnostic.
    try {
        (void)XmlDocument::fromStringOrThrow("<orkester><gruppe></orkester>"_el);
    } catch (const el::err::ParseError &) {
        el::io::printLine("Invalid XML document"_el);
    }
}

/// Build an XML document with nested elements and attributes.
///
/// `addRoot()` establishes the document element, and `addElement()` appends children in order. Attributes are text;
/// `setAttribute()` replaces an existing attribute of the same name without changing its position.
void buildXmlDocument() {
    // Append groups and instruments in the order they should appear in XML.
    auto document = XmlDocument{};
    auto root = document.addRoot("orkester"_el);
    root->setAttribute("sted"_el, "Aarhus"_el);
    auto group = root->addElement("gruppe"_el);
    group->setAttribute("navn"_el, "strygere"_el);
    group->addElement("instrument"_el, "violin"_el);
    group->addElement("instrument"_el, "cello"_el);
    el::io::printLine("Groups: "_el, root->children().count().toSizeT());
    el::io::printLine("Instruments: "_el, group->children().count().toSizeT());
}

/// Prepare a document with namespace spelling and ordered markup nodes.
///
/// Namespace declarations are ordinary attributes in this DOM. Factories create special node kinds, and `add()`
/// places them at the intended document or element position. Declaration and DOCTYPE text includes its full markup.
void buildXmlMarkup() {
    // Place document-level markup before the root, then append mixed nodes to it.
    auto document = XmlDocument{};
    document.add(XmlNode::createDeclaration("<?xml version=\"1.0\"?>"_el));
    document.add(XmlNode::createDocType("<!DOCTYPE musik:orkester>"_el));
    auto root = document.addRoot("musik:orkester"_el);
    root->setAttribute("xmlns:musik"_el, "urn:musik"_el);
    root->add(XmlNode::createComment(" første sats "_el));
    root->add(XmlNode::createProcessingInstruction("spil"_el, " langsomt"_el));
    root->add(XmlNode::createCData("violin & bratsch"_el));
    const auto xml = document.toString();
    el::io::printLine("DOCTYPE written: "_el, xml.contains("<!DOCTYPE musik:orkester>"_el));
    el::io::printLine("CDATA written: "_el, xml.contains("<![CDATA[violin & bratsch]]>"_el));
    el::io::printLine("Parsed root: "_el, XmlDocument::fromStringOrThrow(xml).root()->name());
}

/// Serialize a document as XML text and parse the result.
///
/// `toString()` escapes ordinary text and attributes. It writes an empty element with `/>` and retains the order of
/// document nodes, children, and attributes, while its choice of quotes and whitespace need not match source text.
void serializeXmlDocument() {
    // The writer escapes ordinary text, and the parser restores its value.
    auto document = XmlDocument{};
    auto root = document.addRoot("instrument"_el, "violin & bratsch"_el);
    root->setAttribute("gruppe"_el, "strygere"_el);
    const auto xml = document.toString();
    el::io::printLine("XML: "_el, xml);
    el::io::printLine("Text: "_el, XmlDocument::fromStringOrThrow(xml).root()->textContentOrThrow());
}

/// Inspect ordered XML nodes and their decoded text after parsing.
///
/// `children()` retains mixed content in document order. `getAttribute()` and `textContent()` return optional decoded
/// text, so callers can distinguish an absent or unresolved value from ordinary text.
void inspectXmlDocument() {
    // Mixed content retains text and elements as separate ordered children.
    const auto source =
        "<gruppe navn=\"træblæsere\">Solo: <instrument>fløjte</instrument> og <instrument>obo</instrument></gruppe>"_el;
    const auto document = XmlDocument::fromStringOrThrow(source);
    const auto root = document.root();
    el::io::printLine("Group: "_el, root->getAttribute("navn"_el).value());
    el::io::printLine("First child is text: "_el, root->children().get(el::ItemIndex{0U})->type() == XmlNodeType::Text);
    el::io::printLine("First instrument: "_el, root->children().get(el::ItemIndex{1U})->textContentOrThrow());
    el::io::printLine("All text: "_el, root->textContentOrThrow());
}

/// Preserve XML markup whose entity values are unavailable.
///
/// XML declarations, DOCTYPE declarations, comments, CDATA, instructions, and references remain distinct nodes.
/// External resources and DTD entities are not resolved, so `textContent()` can be empty even for valid XML.
void inspectXmlMarkup() {
    // The DTD entity is retained as a reference, including inside the attribute.
    const auto source =
        "<?xml version=\"1.0\"?><!DOCTYPE orkester [<!ENTITY leder \"Maja\">]>"
        "<orkester><gruppe leder=\"&leder;\"><![CDATA[fløjte & obo]]><!-- solo -->&leder;</gruppe></orkester>"_el;
    const auto document = XmlDocument::fromStringOrThrow(source);
    const auto group = document.root()->children().get(el::ItemIndex{0U});
    el::io::printLine("Top-level nodes: "_el, document.nodes().count().toSizeT());
    el::io::printLine(
        "First child is CDATA: "_el, group->children().get(el::ItemIndex{0U})->type() == XmlNodeType::CData);
    el::io::printLine("Known text: "_el, group->children().get(el::ItemIndex{0U})->textContentOrThrow());
    el::io::printLine("Attribute available: "_el, group->getAttribute("leder"_el).has_value());
    el::io::printLine("All text available: "_el, group->textContent().has_value());
    try {
        (void)group->textContentOrThrow();
    } catch (const el::err::LogicError &) {
        el::io::printLine("Unresolved entity reference"_el);
    }
    el::io::printLine("DOCTYPE retained: "_el, document.toString().contains("<!DOCTYPE"_el));
}

/// Apply the four independent XML parser limits to one document.
///
/// The limits cover complete source bytes, element depth, total nodes, and each name or text value. A limit violation
/// rejects the document through either parser form.
void limitXmlInput() {
    // Each limit rejects the same source for an independent reason.
    const auto source = "<orkester><gruppe navn=\"strygere\"><instrument>violin</instrument></gruppe></orkester>"_el;
    const auto input = XmlParseOptions{}.setMaximumInputLength(el::ByteLength{10U});
    const auto nesting = XmlParseOptions{}.setMaximumNesting(el::ItemCount{2U});
    const auto nodes = XmlParseOptions{}.setMaximumNodeCount(el::ItemCount{2U});
    const auto strings = XmlParseOptions{}.setMaximumStringLength(el::ByteLength{4U});
    el::io::printLine("Input accepted: "_el, XmlDocument::fromString(source, input).has_value());
    el::io::printLine("Nesting accepted: "_el, XmlDocument::fromString(source, nesting).has_value());
    el::io::printLine("Nodes accepted: "_el, XmlDocument::fromString(source, nodes).has_value());
    el::io::printLine("Strings accepted: "_el, XmlDocument::fromString(source, strings).has_value());
}

}
