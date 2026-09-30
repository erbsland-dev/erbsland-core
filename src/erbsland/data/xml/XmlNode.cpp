// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "XmlNode.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../text/AnyString.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/u8/U8StringConstIterator.hpp"

#include <limits>

namespace erbsland::data::xml {

using namespace text::literals;

auto XmlNode::createElement(text::String name, text::String text) -> XmlNodePtr {
    auto result = XmlNodePtr{new XmlNode{XmlNodeType::Element, std::move(name), {}}};
    if (!text.isEmpty()) {
        result->addText(std::move(text));
    }
    return result;
}

auto XmlNode::createText(text::String text) -> XmlNodePtr {
    return XmlNodePtr{new XmlNode{XmlNodeType::Text, {}, std::move(text)}};
}

auto XmlNode::createCData(text::String text) -> XmlNodePtr {
    return XmlNodePtr{new XmlNode{XmlNodeType::CData, {}, std::move(text)}};
}

auto XmlNode::createComment(text::String text) -> XmlNodePtr {
    return XmlNodePtr{new XmlNode{XmlNodeType::Comment, {}, std::move(text)}};
}

auto XmlNode::createProcessingInstruction(text::String name, text::String text) -> XmlNodePtr {
    return XmlNodePtr{new XmlNode{XmlNodeType::ProcessingInstruction, std::move(name), std::move(text)}};
}

auto XmlNode::createEntityReference(text::String name) -> XmlNodePtr {
    return XmlNodePtr{new XmlNode{XmlNodeType::EntityReference, std::move(name), {}}};
}

auto XmlNode::createDocType(text::String raw) -> XmlNodePtr {
    return XmlNodePtr{new XmlNode{XmlNodeType::DocType, {}, std::move(raw)}};
}

auto XmlNode::createDeclaration(text::String raw) -> XmlNodePtr {
    return XmlNodePtr{new XmlNode{XmlNodeType::Declaration, {}, std::move(raw)}};
}

auto XmlNode::resolveReference(const text::String &name) -> std::optional<text::String> {
    if (name == "amp"_el) {
        return "&"_el;
    }
    if (name == "lt"_el) {
        return "<"_el;
    }
    if (name == "gt"_el) {
        return ">"_el;
    }
    if (name == "quot"_el) {
        return "\""_el;
    }
    if (name == "apos"_el) {
        return "'"_el;
    }
    if (!name.startsWith("#"_el)) {
        return std::nullopt;
    }
    const auto hexadecimal = name.startsWith("#x"_el) || name.startsWith("#X"_el);
    const auto skip = hexadecimal ? 2U : 1U;
    auto value = uint32_t{};
    auto count = std::size_t{};
    for (const auto character : name) {
        if (count++ < skip) {
            continue;
        }
        const auto digit = character.digitValue();
        if (!digit || *digit >= static_cast<unsigned>(hexadecimal ? 16 : 10) ||
            value > (0x10ffffU - *digit) / static_cast<uint32_t>(hexadecimal ? 16 : 10)) {
            return std::nullopt;
        }
        value = value * static_cast<uint32_t>(hexadecimal ? 16 : 10) + *digit;
    }
    if (count <= skip || value == 0U || value > 0x10ffffU || (value >= 0xd800U && value <= 0xdfffU) ||
        (value < 0x20U && value != 0x09U && value != 0x0aU && value != 0x0dU)) {
        return std::nullopt;
    }
    auto builder = text::AnyStringBuilder{};
    builder.append(static_cast<char32_t>(value));
    return builder.takeString();
}

auto XmlNode::resolveAttribute(const XmlAttribute &attribute) -> std::optional<text::String> {
    if (!attribute.hasReferences()) {
        return attribute.value();
    }
    auto reader = text::StringCharReader{attribute.value()};
    auto builder = text::AnyStringBuilder{};
    while (!reader.isAtEnd()) {
        if (!reader.advanceIf(U'&')) {
            builder.append(reader.read());
            continue;
        }
        reader.clearBuffer();
        while (!reader.isAtEnd() && reader.peek() != U';') {
            reader.readToBuffer();
        }
        if (!reader.advanceIf(U';')) {
            return std::nullopt;
        }
        const auto resolved = resolveReference(reader.takeBuffer().toString());
        if (!resolved) {
            return std::nullopt;
        }
        builder.append(*resolved);
    }
    return builder.takeString();
}

auto XmlNode::getAttribute(const text::String &name) const -> std::optional<text::String> {
    for (const auto &attribute : _attributes) {
        if (attribute.name() == name) {
            return resolveAttribute(attribute);
        }
    }
    return std::nullopt;
}

auto XmlNode::appendTextContent(text::AnyStringBuilder &builder) const -> bool {
    switch (_type) {
    case XmlNodeType::Text:
    case XmlNodeType::CData:
        builder.append(_text);
        return true;
    case XmlNodeType::EntityReference: {
        const auto resolved = resolveReference(_name);
        if (!resolved) {
            return false;
        }
        builder.append(*resolved);
        return true;
    }
    case XmlNodeType::Element:
        for (const auto &child : _children) {
            if (!child->appendTextContent(builder)) {
                return false;
            }
        }
        return true;
    default:
        return true;
    }
}

auto XmlNode::textContent() const -> std::optional<text::String> {
    auto builder = text::AnyStringBuilder{};
    if (!appendTextContent(builder)) {
        return std::nullopt;
    }
    return builder.takeString();
}

auto XmlNode::textContentOrThrow() const -> text::String {
    const auto result = textContent();
    if (!result) {
        throw err::LogicError{"XML text contains an unresolved entity reference."_el};
    }
    return *result;
}

auto XmlNode::setAttribute(text::String name, text::String value) -> XmlNode & {
    if (_type != XmlNodeType::Element) {
        throw err::LogicError{"Only XML elements have attributes."_el};
    }
    for (auto i = unit::ItemIndex{}; i.isWithin(_attributes.count()); ++i) {
        if (_attributes.getRefOrThrow(i).name() == name) {
            _attributes.set(i, XmlAttribute{std::move(name), std::move(value)});
            return *this;
        }
    }
    _attributes.append(XmlAttribute{std::move(name), std::move(value)});
    return *this;
}

auto XmlNode::add(XmlNodePtr child) -> XmlNodePtr {
    if (_type != XmlNodeType::Element || !child) {
        throw err::ParameterError{"Invalid XML child insertion."_el, "child"_el};
    }
    _children.append(child);
    return child;
}

auto XmlNode::addElement(text::String name, text::String text) -> XmlNodePtr {
    return add(createElement(std::move(name), std::move(text)));
}

auto XmlNode::addText(text::String text) -> XmlNodePtr {
    return add(createText(std::move(text)));
}

}
