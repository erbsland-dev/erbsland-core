// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Lexer.hpp"

#include "Core.hpp"
#include "Number.hpp"
#include "Section.hpp"
#include "Value.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../../text/StringFormat.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

auto Lexer::sourceIdentifier() const noexcept -> SourceIdentifierPtr {
    if (_decoder == nullptr) {
        return {};
    }
    return _decoder->sourceIdentifier();
}

auto Lexer::tokens() -> TokenGenerator {
    try {
        if (_decoder == nullptr) {
            throw ConfError{ConfErrorCategory::Internal, "You cannot read from a closed lexer."_el};
        }
        decoder().initialize();
        // This is the main state. We assume that the read character is always the first character of a new line.
        while (!decoder().character().isEndOfData()) {
            if (decoder().character() == CharClass::Spacing) {
                // Manually handle spacing to improve the error reporting.
                EL_YIELD(lexer::expectSpacing(decoder())); // read the spacing
                // Now see what we get at this point
                if (decoder().character() == CharClass::EndOfLineStart) {
                    EL_YIELD_FROM(lexer::expectEndOfLine(decoder(), lexer::ExpectMore::No));
                } else {
                    if (decoder().character() == CharClass::NameStart) {
                        decoder().throwSyntaxError(
                            "Value names must appear at the beginning of a line without leading spaces."_el);
                    }
                    if (decoder().character() == CharClass::SectionStart) {
                        decoder().throwSyntaxError(
                            "Section declarations must start at the beginning of a line without any indentation."_el);
                    }
                    decoder().throwSyntaxOrUnexpectedEndError(
                        u8"Unexpected content after indentation: only a comments or an empty lines was expected at "
                        "this point."_el);
                }
            }
            if (decoder().character() == CharClass::EndOfLineStart) {
                EL_YIELD_FROM(lexer::expectEndOfLine(decoder(), lexer::ExpectMore::No));
            } else if (decoder().character() == CharClass::NameStart) {
                EL_YIELD_FROM(lexer::expectNameAndValue(decoder()));
            } else if (decoder().character() == CharClass::SectionStart) { // We got any character that potentially
                                                                           // starts a section
                EL_YIELD_FROM(lexer::expectSection(decoder()));
            } else {
                decoder().throwSyntaxError("Expected a section, name or empty line, but got something else."_el);
            }
        }
        // Always return an end of data token as the last token in the stream.
        EL_YIELD(decoder().createEndOfDataToken());
        close();
        co_return;
    } catch (const ConfError &) {
        close();
        throw;
    }
}

auto Lexer::digest() const -> mem::ByteBlock {
    return _digest;
}

auto Lexer::hashAlgorithm() -> cryptology::HashAlgorithm {
    return defaults::documentHashAlgorithm;
}

void Lexer::close() noexcept {
    // Store the digest before the decoder is deleted.
    if (_decoder != nullptr) {
        _digest = _decoder->digest();
    }
    _decoder.reset();
}

#ifdef ERBSLAND_CORE_CONF_INTERNAL_VIEWS
namespace {
auto escapeRawText(const text::String &str) noexcept -> text::String {
    auto result = text::StringEditor{};
    auto reader = text::StringCharReader{str};
    for (auto character = reader.read(); character != text::Char::endOfData(); character = reader.read()) {
        if (character == '"') {
            result.append("\\\""_el);
        } else if (character == '\\') {
            result.append("\\\\"_el);
        } else if (character == '\n') {
            result.append("\\n"_el);
        } else if (character == '\r') {
            result.append("\\r"_el);
        } else if (character == '\t') {
            result.append("\\t"_el);
        } else {
            result.append(character);
        }
    }
    return result;
}
template <typename T>
auto visualizeValue(const T & /*value*/) noexcept -> text::String {
    throw err::LogicError("Not implemented");
}
template <>
auto visualizeValue(const NoContent & /*value*/) noexcept -> text::String {
    return "No Content"_el;
}
template <>
auto visualizeValue(const Integer &value) noexcept -> text::String {
    return text::StringFormat{"Integer: {}"_el}.build(value);
}
template <>
auto visualizeValue(const Float &value) noexcept -> text::String {
    return text::StringFormat{"Float: {}"_el}.build(value);
}
template <>
auto visualizeValue(const bool &value) noexcept -> text::String {
    return text::StringFormat{"Boolean: {}"_el}.build(value);
}
template <>
auto visualizeValue(const text::String &value) noexcept -> text::String {
    auto escapedText = text::StringEditor{"String: \""_el};
    escapedText.append(escapeRawText(value));
    escapedText.append('"');
    return escapedText;
}
template <>
auto visualizeValue(const time::Date &value) noexcept -> text::String {
    return text::StringFormat{"Date: {}"_el}.build(value.toString());
}
template <>
auto visualizeValue(const time::Time &value) noexcept -> text::String {
    return text::StringFormat{"Time: {}"_el}.build(value.toString());
}
template <>
auto visualizeValue(const time::TimeWithZone &value) noexcept -> text::String {
    return text::StringFormat{"Time: {}"_el}.build(value.toString());
}
template <>
auto visualizeValue(const time::DateTime &value) noexcept -> text::String {
    return text::StringFormat{"DateTime: {}"_el}.build(value.toString());
}
template <>
auto visualizeValue(const mem::ByteBlock &value) noexcept -> text::String {
    auto hexText = text::StringEditor{"Bytes: "_el};
    const auto displayElements = std::min(std::size_t{32}, value.length().toSizeT());
    for (std::size_t i = 0; i < displayElements; ++i) {
        hexText.append(text::StringFormat{"{:02x} "_el}.build(value.get(unit::ByteIndex{i}).toUInt8()));
    }
    if (value.length().toSizeT() > displayElements) {
        hexText.append(text::StringFormat{"... ({} more bytes)"_el}.build(value.length().toSizeT() - displayElements));
    }
    return hexText;
}
template <>
auto visualizeValue(const time::CalendarDelta &value) noexcept -> text::String {
    return text::StringFormat{"TimeDelta: {}"_el}.build(value.toString(time::TimeDeltaFormat::elcl()));
}
}

auto internalView(const Lexer &object) -> InternalViewPtr {
    auto result = InternalView::create();
    if (object._decoder != nullptr) {
        result->setValue("decoder"_el, *object._decoder);
    } else {
        result->setValue("decoder"_el, text::String{"null"_el});
    }
    return result;
}

// The internal view of the LexerToken is defined here avoiding linker warnings about empty units.
auto internalView(const LexerToken &token) noexcept -> InternalViewPtr {
    auto result = InternalView::create();
    result->setValue("type"_el, toString(token._type));
    if (token._begin.isUndefined()) {
        result->setValue("begin"_el, text::String{"undefined"_el});
    } else {
        result->setValue("begin"_el, token._begin.toString());
    }
    if (token._end.isUndefined()) {
        result->setValue("end"_el, text::String{"undefined"_el});
    } else {
        result->setValue("end"_el, token._end.toString());
    }
    auto rawText = text::StringEditor{"\""_el};
    rawText.append(escapeRawText(token._rawText));
    rawText.append('"');
    result->setValue("rawText"_el, text::String{rawText});
    std::visit([&](auto &&value) { result->setValue("value"_el, visualizeValue(value)); }, token._content);
    return result;
}
#endif

}
