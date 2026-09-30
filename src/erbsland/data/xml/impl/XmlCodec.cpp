// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "XmlCodec.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../err/ParseError.hpp"
#include "../../../text/AnyString.hpp"
#include "../../../text/CaseSensitivity.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/u8/U8StringConstIterator.hpp"

#include <compare>

namespace erbsland::data::xml::impl {

using namespace text::literals;

XmlCodec::XmlCodec(text::String source, XmlParseOptions options) :
    _source{std::move(source)}, _reader{_source}, _parseOptions{options} {
}

XmlCodec::XmlCodec([[maybe_unused]] XmlFormatOptions options) {
}

auto XmlCodec::decode(const text::String &source, XmlParseOptions options) -> XmlDocument {
    return XmlCodec{source, options}.parse();
}

auto XmlCodec::encode(const XmlDocument &document, XmlFormatOptions options) -> text::String {
    auto codec = XmlCodec{options};
    auto seenRoot = false;
    auto seenDocType = false;
    auto seenAnyNode = false;
    for (const auto &node : document.nodes()) {
        switch (node->type()) {
        case XmlNodeType::Element:
            if (seenRoot) {
                throw err::ParameterError{"XML document has more than one root."_el, "document"_el};
            }
            seenRoot = true;
            break;
        case XmlNodeType::DocType:
            if (seenRoot || seenDocType) {
                throw err::ParameterError{"DOCTYPE must occur once before the XML root."_el, "document"_el};
            }
            seenDocType = true;
            break;
        case XmlNodeType::Declaration:
            if (seenAnyNode) {
                throw err::ParameterError{"XML declaration must be the first node."_el, "document"_el};
            }
            break;
        case XmlNodeType::Text:
            for (const auto character : node->text()) {
                if (character != U' ' && character != U'\t' && character != U'\r' && character != U'\n') {
                    throw err::ParameterError{"Top-level XML text must be whitespace."_el, "document"_el};
                }
            }
            break;
        case XmlNodeType::Comment:
        case XmlNodeType::ProcessingInstruction:
            break;
        default:
            throw err::ParameterError{"Invalid top-level XML node."_el, "document"_el};
        }
        seenAnyNode = true;
        codec.writeNode(*node);
    }
    if (!seenRoot) {
        throw err::ParameterError{"XML document requires one root element."_el, "document"_el};
    }
    return codec._builder.takeString();
}

void XmlCodec::skipWhitespace() {
    while (
        !_reader.isAtEnd() &&
        (_reader.peek() == U' ' || _reader.peek() == U'\t' || _reader.peek() == U'\r' || _reader.peek() == U'\n')) {
        _reader.advance();
    }
}

void XmlCodec::countNode() {
    if (_nodeCount >= _parseOptions.maximumNodeCount()) {
        fail();
    }
    ++_nodeCount;
}

auto XmlCodec::validNameChar(text::Char character, bool first) noexcept -> bool {
    if (character.isAsciiLetter() || character == U'_' || character == U':') {
        return true;
    }
    if (!first && (character.isAsciiDigit() || character == U'-' || character == U'.')) {
        return true;
    }
    const auto code = character.toRawValue();
    const auto nameStart = (code >= 0xc0U && code <= 0xd6U) || (code >= 0xd8U && code <= 0xf6U) ||
        (code >= 0xf8U && code <= 0x2ffU) || (code >= 0x370U && code <= 0x37dU) ||
        (code >= 0x37fU && code <= 0x1fffU) || (code >= 0x200cU && code <= 0x200dU) ||
        (code >= 0x2070U && code <= 0x218fU) || (code >= 0x2c00U && code <= 0x2fefU) ||
        (code >= 0x3001U && code <= 0xd7ffU) || (code >= 0xf900U && code <= 0xfdcfU) ||
        (code >= 0xfdf0U && code <= 0xfffdU) || (code >= 0x10000U && code <= 0xeffffU);
    return nameStart ||
        (!first && (code == 0xb7U || (code >= 0x300U && code <= 0x36fU) || (code >= 0x203fU && code <= 0x2040U)));
}

auto XmlCodec::validXmlChar(text::Char character) noexcept -> bool {
    const auto code = character.toRawValue();
    return code == 0x9U || code == 0xaU || code == 0xdU || (code >= 0x20U && code <= 0xd7ffU) ||
        (code >= 0xe000U && code <= 0xfffdU) || (code >= 0x10000U && code <= 0x10ffffU);
}

auto XmlCodec::validName(const text::String &value) -> bool {
    auto first = true;
    for (const auto character : value) {
        if (!validNameChar(character, first)) {
            return false;
        }
        first = false;
    }
    return !first;
}

auto XmlCodec::readName() -> text::String {
    _reader.clearBuffer();
    auto first = true;
    while (!_reader.isAtEnd() && validNameChar(_reader.peek(), first)) {
        if (!validXmlChar(_reader.peek())) {
            fail();
        }
        (void)_reader.readToBuffer();
        first = false;
    }
    if (first) {
        fail();
    }
    const auto name = _reader.takeBuffer().toString();
    if (name.length() > _parseOptions.maximumStringLength()) {
        fail();
    }
    return name;
}

auto XmlCodec::readUntil(const text::String &terminator) -> text::String {
    _reader.clearBuffer();
    while (!_reader.isAtEnd()) {
        if (_reader.advanceIf(terminator)) {
            const auto result = _reader.takeBuffer().toString();
            if (result.length() > _parseOptions.maximumStringLength()) {
                fail();
            }
            return result;
        }
        if (!validXmlChar(_reader.peek())) {
            fail();
        }
        (void)_reader.readToBuffer();
    }
    fail();
}

auto XmlCodec::readComment() -> XmlNodePtr {
    const auto content = readUntil("-->"_el);
    if (content.contains("--"_el) || content.endsWith("-"_el)) {
        fail();
    }
    countNode();
    return XmlNode::createComment(content);
}

auto XmlCodec::readCData() -> XmlNodePtr {
    const auto content = readUntil("]]>"_el);
    countNode();
    return XmlNode::createCData(content);
}

auto XmlCodec::readInstruction(bool declaration) -> XmlNodePtr {
    const auto name = readName();
    const auto content = readUntil("?>"_el);
    countNode();
    if (declaration) {
        return XmlNode::createDeclaration(text::String::fromJoined({"<?"_el, name, content, "?>"_el}));
    }
    if (name.compare("xml"_el, text::cCaseInsensitive.asciiComparisonFn()) == std::strong_ordering::equal) {
        fail();
    }
    return XmlNode::createProcessingInstruction(name, content);
}

auto XmlCodec::readDocType() -> XmlNodePtr {
    _reader.clearBuffer();
    auto bracketDepth = uint32_t{};
    auto quote = char32_t{};
    auto inComment = false;
    auto inInstruction = false;
    auto previous = char32_t{};
    auto previousPrevious = char32_t{};
    while (!_reader.isAtEnd()) {
        const auto character = _reader.read();
        if (!validXmlChar(character)) {
            fail();
        }
        if (inComment) {
            if (previousPrevious == U'-' && previous == U'-' && character == U'>') {
                inComment = false;
            }
        } else if (inInstruction) {
            if (previous == U'?' && character == U'>') {
                inInstruction = false;
            }
        } else if (quote == 0U && character == U'<' && _reader.advanceIf("!--"_el)) {
            _reader.appendToBuffer(character);
            _reader.appendToBuffer(U'!');
            _reader.appendToBuffer(U'-');
            _reader.appendToBuffer(U'-');
            inComment = true;
            previousPrevious = previous = U'-';
            continue;
        } else if (quote == 0U && character == U'<' && _reader.advanceIf(U'?')) {
            _reader.appendToBuffer(character);
            _reader.appendToBuffer(U'?');
            inInstruction = true;
            previousPrevious = U'<';
            previous = U'?';
            continue;
        } else if (quote != 0U) {
            if (character == quote) {
                quote = 0U;
            }
        } else if (character == U'\'' || character == U'"') {
            quote = character.toRawValue();
        } else if (character == U'[') {
            ++bracketDepth;
        } else if (character == U']' && bracketDepth > 0U) {
            --bracketDepth;
        } else if (character == U'>' && bracketDepth == 0U) {
            countNode();
            const auto content = _reader.takeBuffer().toString();
            if (content.length() > _parseOptions.maximumStringLength()) {
                fail();
            }
            return XmlNode::createDocType(text::String::fromJoined({"<!DOCTYPE"_el, content, ">"_el}));
        }
        _reader.appendToBuffer(character);
        previousPrevious = previous;
        previous = character.toRawValue();
    }
    fail();
}

auto XmlCodec::readReference() -> XmlNodePtr {
    _reader.clearBuffer();
    while (!_reader.isAtEnd() && _reader.peek() != U';') {
        const auto character = _reader.peek();
        if (character == U'<' || character == U'&' || character == U' ' || character == U'\n' || character == U'\t') {
            fail();
        }
        (void)_reader.readToBuffer();
    }
    if (!_reader.advanceIf(U';')) {
        fail();
    }
    const auto name = _reader.takeBuffer().toString();
    if (name.length() > _parseOptions.maximumStringLength() || name.isEmpty() ||
        (name.startsWith("#"_el) ? !XmlNode::resolveReference(name) : !validName(name))) {
        fail();
    }
    countNode();
    return XmlNode::createEntityReference(name);
}

auto XmlCodec::readText() -> XmlNodePtr {
    _reader.clearBuffer();
    while (!_reader.isAtEnd() && _reader.peek() != U'<' && _reader.peek() != U'&') {
        if (!validXmlChar(_reader.peek())) {
            fail();
        }
        (void)_reader.readToBuffer();
    }
    const auto text = _reader.takeBuffer().toString();
    if (text.length() > _parseOptions.maximumStringLength() || text.contains("]]>"_el)) {
        fail();
    }
    countNode();
    return XmlNode::createText(text);
}

void XmlCodec::readAttribute(XmlNode &node) {
    const auto name = readName();
    for (const auto &attribute : node._attributes) {
        if (attribute.name() == name) {
            fail();
        }
    }
    skipWhitespace();
    if (!_reader.advanceIf(U'=')) {
        fail();
    }
    skipWhitespace();
    const auto quote = _reader.read();
    if (quote != U'\'' && quote != U'"') {
        fail();
    }
    _reader.clearBuffer();
    auto hasReferences = false;
    auto inReference = false;
    auto referenceLength = unit::CpLength{};
    auto referenceNumeric = false;
    auto referenceHex = false;
    auto numericDigits = uint32_t{};
    auto numericValue = uint32_t{};
    while (!_reader.isAtEnd() && _reader.peek() != quote) {
        const auto character = _reader.read();
        if (!validXmlChar(character) || character == U'<') {
            fail();
        }
        if (character == U'&') {
            if (inReference) {
                fail();
            }
            inReference = true;
            hasReferences = true;
            referenceLength = {};
            referenceNumeric = false;
            referenceHex = false;
            numericDigits = 0U;
            numericValue = 0U;
        } else if (character == U';' && inReference) {
            if (referenceLength.isZero() ||
                (referenceNumeric &&
                    (numericDigits == 0U || !validXmlChar(text::Char{static_cast<char32_t>(numericValue)})))) {
                fail();
            }
            inReference = false;
        } else if (inReference) {
            if (referenceLength.isZero() && character == U'#') {
                referenceNumeric = true;
            } else if (referenceNumeric) {
                if (referenceLength == unit::CpLength{1U} && character == U'x') {
                    referenceHex = true;
                } else {
                    const auto code = character.toRawValue();
                    auto digit = uint32_t{};
                    if (code >= U'0' && code <= U'9') {
                        digit = code - U'0';
                    } else if (referenceHex && code >= U'a' && code <= U'f') {
                        digit = code - U'a' + 10U;
                    } else if (referenceHex && code >= U'A' && code <= U'F') {
                        digit = code - U'A' + 10U;
                    } else {
                        fail();
                    }
                    const auto base = referenceHex ? 16U : 10U;
                    if (numericValue > (0x10ffffU - digit) / base) {
                        fail();
                    }
                    numericValue = numericValue * base + digit;
                    ++numericDigits;
                }
            } else if (!validNameChar(character, referenceLength.isZero())) {
                fail();
            }
            ++referenceLength;
        }
        _reader.appendToBuffer(character);
    }
    if (inReference || !_reader.advanceIf(quote)) {
        fail();
    }
    const auto value = _reader.takeBuffer().toString();
    if (value.length() > _parseOptions.maximumStringLength()) {
        fail();
    }
    auto attribute = XmlAttribute{name, value};
    attribute._hasReferences = hasReferences;
    node._attributes.append(std::move(attribute));
}

auto XmlCodec::readElement(unit::ItemCount depth) -> XmlNodePtr {
    if (depth >= _parseOptions.maximumNesting()) {
        fail();
    }
    const auto name = readName();
    auto node = XmlNode::createElement(name);
    countNode();
    while (true) {
        const auto before = _reader.position();
        skipWhitespace();
        if (_reader.advanceIf("/>"_el)) {
            return node;
        }
        if (_reader.advanceIf(U'>')) {
            break;
        }
        if (before == _reader.position()) {
            fail();
        }
        readAttribute(*node);
    }
    while (!_reader.isAtEnd()) {
        if (_reader.advanceIf("</"_el)) {
            if (readName() != name) {
                fail();
            }
            skipWhitespace();
            if (!_reader.advanceIf(U'>')) {
                fail();
            }
            return node;
        }
        if (_reader.advanceIf("<!--"_el)) {
            node->add(readComment());
        } else if (_reader.advanceIf("<![CDATA["_el)) {
            node->add(readCData());
        } else if (_reader.advanceIf("<?"_el)) {
            node->add(readInstruction(false));
        } else if (_reader.advanceIf(U'<')) {
            node->add(readElement(depth + unit::ItemCount::one()));
        } else if (_reader.advanceIf(U'&')) {
            node->add(readReference());
        } else {
            node->add(readText());
        }
    }
    fail();
}

auto XmlCodec::parse() -> XmlDocument {
    if (_source.length() > _parseOptions.maximumInputLength()) {
        fail();
    }
    auto document = XmlDocument{};
    if (_reader.peek() == U'\ufeff') {
        _reader.advance();
    }
    const auto declarationStart = _reader.save();
    if (_reader.advanceIf("<?xml"_el) &&
        (_reader.peek() == U' ' || _reader.peek() == U'\t' || _reader.peek() == U'\r' || _reader.peek() == U'\n')) {
        const auto content = readUntil("?>"_el);
        countNode();
        document.add(XmlNode::createDeclaration(text::String::fromJoined({"<?xml"_el, content, "?>"_el})));
    } else {
        _reader.restore(declarationStart);
    }
    auto seenRoot = false;
    auto seenDocType = false;
    while (!_reader.isAtEnd()) {
        if (_reader.advanceIf("<!--"_el)) {
            document.add(readComment());
        } else if (_reader.advanceIf("<?"_el)) {
            document.add(readInstruction(false));
        } else if (_reader.advanceIf("<!DOCTYPE"_el)) {
            if (seenRoot || seenDocType) {
                fail();
            }
            document.add(readDocType());
            seenDocType = true;
        } else if (_reader.advanceIf(U'<')) {
            if (seenRoot) {
                fail();
            }
            document.add(readElement({}));
            seenRoot = true;
        } else {
            if (_reader.peek() == U'&') {
                fail();
            }
            const auto text = readText();
            for (const auto character : text->text()) {
                if (character != U' ' && character != U'\t' && character != U'\r' && character != U'\n') {
                    fail();
                }
            }
            document.add(text);
        }
    }
    if (!seenRoot) {
        fail();
    }
    return document;
}

[[noreturn]] void XmlCodec::fail() {
    throw err::ParseError{"Malformed or unsupported XML document."_el};
}

}
