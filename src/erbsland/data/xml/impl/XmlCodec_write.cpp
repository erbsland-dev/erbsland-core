// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "XmlCodec.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/u8/U8StringConstIterator.hpp"

#include <compare>

namespace erbsland::data::xml::impl {

using namespace text::literals;

void XmlCodec::validateXmlText(const text::String &value) {
    for (const auto character : value) {
        if (!validXmlChar(character)) {
            throw err::ParameterError{"XML text contains a forbidden character."_el, "value"_el};
        }
    }
}

void XmlCodec::writeEscaped(const text::String &value, bool attribute, bool preserveReferences) {
    validateXmlText(value);
    auto inReference = false;
    for (const auto character : value) {
        if (preserveReferences && character == U'&') {
            inReference = true;
            _builder.append(character);
            continue;
        }
        if (inReference) {
            _builder.append(character);
            if (character == U';') {
                inReference = false;
            }
            continue;
        }
        if (character == U'&') {
            _builder.append("&amp;"_el);
        } else if (character == U'<') {
            _builder.append("&lt;"_el);
        } else if (character == U'>') {
            _builder.append("&gt;"_el);
        } else if (attribute && character == U'"') {
            _builder.append("&quot;"_el);
        } else {
            _builder.append(character);
        }
    }
}

void XmlCodec::writeNode(const XmlNode &node) {
    switch (node.type()) {
    case XmlNodeType::Element:
        if (!validName(node.name())) {
            throw err::ParameterError{"Invalid XML element name."_el, "name"_el};
        }
        _builder.append(U'<').append(node.name());
        for (const auto &attribute : node.attributes()) {
            if (!validName(attribute.name())) {
                throw err::ParameterError{"Invalid XML attribute name."_el, "name"_el};
            }
            _builder.append(U' ').append(attribute.name()).append("=\""_el);
            writeEscaped(attribute.value(), true, attribute.hasReferences());
            _builder.append(U'"');
        }
        if (node.children().isEmpty()) {
            _builder.append("/>"_el);
            break;
        }
        _builder.append(U'>');
        for (const auto &child : node.children()) {
            if (child->type() == XmlNodeType::DocType || child->type() == XmlNodeType::Declaration) {
                throw err::ParameterError{"XML declarations cannot occur inside an element."_el, "node"_el};
            }
            writeNode(*child);
        }
        _builder.append("</"_el).append(node.name()).append(U'>');
        break;
    case XmlNodeType::Text:
        writeEscaped(node.text(), false);
        break;
    case XmlNodeType::CData:
        validateXmlText(node.text());
        if (node.text().contains("]]>"_el)) {
            throw err::ParameterError{"CDATA contains its closing delimiter."_el, "node"_el};
        }
        _builder.append("<![CDATA["_el).append(node.text()).append("]]>"_el);
        break;
    case XmlNodeType::Comment:
        validateXmlText(node.text());
        if (node.text().contains("--"_el) || node.text().endsWith("-"_el)) {
            throw err::ParameterError{"Invalid XML comment."_el, "node"_el};
        }
        _builder.append("<!--"_el).append(node.text()).append("-->"_el);
        break;
    case XmlNodeType::ProcessingInstruction:
        validateXmlText(node.text());
        if (!validName(node.name()) || node.text().contains("?>"_el) ||
            node.name().compare("xml"_el, text::cCaseInsensitive.asciiComparisonFn()) == std::strong_ordering::equal) {
            throw err::ParameterError{"Invalid XML processing instruction."_el, "node"_el};
        }
        _builder.append("<?"_el).append(node.name()).append(node.text()).append("?>"_el);
        break;
    case XmlNodeType::DocType:
    case XmlNodeType::Declaration:
        validateXmlText(node.text());
        _builder.append(node.text());
        break;
    case XmlNodeType::EntityReference:
        if (node.name().isEmpty() ||
            (node.name().startsWith("#"_el) ? !XmlNode::resolveReference(node.name()) : !validName(node.name()))) {
            throw err::ParameterError{"Invalid XML entity reference."_el, "node"_el};
        }
        _builder.append(U'&').append(node.name()).append(U';');
        break;
    }
}

}
