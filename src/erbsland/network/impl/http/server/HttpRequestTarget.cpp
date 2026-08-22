// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpRequestTarget.hpp"

#include "../../../../err/Exception.hpp"
#include "../../../../err/ParseError.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../text/EncodingMode.hpp"
#include "../../../../text/IntegerBase.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/NormalizationForm.hpp"
#include "../../../../text/StringBomMode.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringDecoder.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../../text/StringEncoding.hpp"

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpRequestTarget::HttpRequestTarget(const String &target) {
    if (!target.isValidUtf8() || !target.startsWith("/"_el) || target.contains("#"_el)) {
        throw err::ParseError{"The HTTP server requires an origin-form request target."_el};
    }
    auto reader = StringCharReader{target};
    reader.advance();
    auto decodedPath = StringEditor{};
    decodedPath.append(U'/');
    auto rawSegment = StringEditor{};
    while (!reader.isAtEnd() && reader.peek() != U'?') {
        const auto character = reader.read();
        if (character == U'/') {
            auto segment = decodeSegment(String{rawSegment});
            _segments.emplace_back(segment);
            decodedPath.append(segment);
            decodedPath.append(U'/');
            rawSegment.clear();
        } else {
            rawSegment.append(character);
        }
    }
    if (!rawSegment.isEmpty() || decodedPath.length() > unit::ByteLength::one()) {
        auto segment = decodeSegment(String{rawSegment});
        _segments.emplace_back(segment);
        decodedPath.append(segment);
    }
    if (reader.advanceIf(U'?')) {
        auto query = StringEditor{};
        while (!reader.isAtEnd()) {
            query.append(reader.read());
        }
        _query = String{query};
    }
    _path = String{decodedPath}.normalized(NormalizationForm::Nfc);
}

auto HttpRequestTarget::decodeSegment(const String &segment) -> String {
    auto reader = StringCharReader{segment};
    auto result = StringEditor{};
    auto encoded = mem::ByteBlockEditor{};
    const auto flushEncoded = [&]() -> void {
        if (encoded.length().isZero()) {
            return;
        }
        try {
            result.append(
                StringDecoder{encoded}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict));
        } catch (const err::Exception &) {
            throw err::ParseError{"A request-target segment contains invalid percent-encoded UTF-8."_el};
        }
        encoded.clear();
    };
    while (!reader.isAtEnd()) {
        if (reader.peek() != U'%') {
            flushEncoded();
            result.append(reader.read());
            continue;
        }
        reader.advance();
        const auto high = reader.read().digitValue(IntegerBase::Hexadecimal);
        const auto low = reader.read().digitValue(IntegerBase::Hexadecimal);
        if (!high.has_value() || !low.has_value()) {
            throw err::ParseError{"A request-target percent escape requires two hexadecimal digits."_el};
        }
        encoded.append(mem::Byte{static_cast<uint8_t>((*high << 4U) | *low)});
    }
    flushEncoded();
    const auto decoded = String{result};
    if (!decoded.isValidUtf8()) {
        throw err::ParseError{"A request-target segment is not valid UTF-8."_el};
    }
    return decoded.normalized(NormalizationForm::Nfc);
}

}
