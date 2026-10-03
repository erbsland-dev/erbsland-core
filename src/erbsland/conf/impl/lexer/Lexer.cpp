// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Lexer.hpp"

#include "DocumentPart.hpp"

#include "../char/NamedChars.hpp"
#include "../utilities/YieldMacros.hpp"

#include "../../../text/StringLiteral.hpp"

namespace erbsland::conf::impl {

using namespace text;
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
            EL_YIELD_FROM(lexer::readDocumentPart(decoder()));
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
auto internalView(const Lexer &object) -> InternalViewPtr {
    auto result = InternalView::create();
    if (object._decoder != nullptr) {
        result->setValue(Lexer::cDecoderKey, *object._decoder);
    } else {
        result->setValue(Lexer::cDecoderKey, "null"_el);
    }
    return result;
}

// The internal view of the LexerToken is defined here avoiding linker warnings about empty units.
auto internalView(const LexerToken &token) noexcept -> InternalViewPtr {
    static const auto quoted = StringFormat{"\"{}\""};
    static const auto typeAndContent = StringFormat{"{}: {}"};
    auto result = InternalView::create();
    result->setValue("type"_el, token._type.toText());
    if (token._begin.isUndefined()) {
        result->setValue(Lexer::cBeginKey, "undefined"_el);
    } else {
        result->setValue(Lexer::cBeginKey, token._begin.toString());
    }
    if (token._end.isUndefined()) {
        result->setValue(Lexer::cEndKey, "undefined"_el);
    } else {
        result->setValue(Lexer::cEndKey, token._end.toString());
    }
    result->setValue("rawText"_el, quoted.build(token._rawText.toEscaped(EscapeFormat::Config)));
    std::visit(
        [&]<typename T>(const T &value) -> void {
            if constexpr (std::is_same_v<T, NoContent>) {
                result->setValue(Lexer::cValueKey, "No Content"_el);
            } else {
                String text;
                String name;
                if constexpr (std::is_same_v<T, String>) {
                    name = "String"_el;
                    text = quoted.build(value.toEscaped(EscapeFormat::Config));
                } else if constexpr (std::is_same_v<T, Integer>) {
                    name = "Integer"_el;
                    text = String::fromInteger(value);
                } else if constexpr (std::is_same_v<T, bool>) {
                    name = "Boolean"_el;
                    text = String::fromBoolean(value);
                } else if constexpr (std::is_same_v<T, Float>) {
                    name = "Float"_el;
                    text = String::fromFloat(value);
                } else if constexpr (std::is_same_v<T, time::Date>) {
                    name = "Date"_el;
                    text = value.toString();
                } else if constexpr (std::is_same_v<T, time::Time> || std::is_same_v<T, time::TimeWithZone>) {
                    name = "Time"_el;
                    text = value.toString();
                } else if constexpr (std::is_same_v<T, time::DateTime>) {
                    name = "DateTime"_el;
                    text = value.toString();
                } else if constexpr (std::is_same_v<T, mem::ByteBlock>) {
                    name = "Bytes"_el;
                    const auto format = ByteFormat::forDiagnostic().setMaximum(unit::ByteCount{32});
                    text = String::fromByteBlock(value, format);
                } else if constexpr (std::is_same_v<T, time::CalendarDelta>) {
                    name = "TimeDelta"_el;
                    text = value.toString(time::TimeDeltaFormat::elcl());
                } else {
                    std::terminate();
                }
                result->setValue(Lexer::cValueKey, typeAndContent.build(name, text));
            }
        },
        token._content);
    return result;
}

#endif

}
