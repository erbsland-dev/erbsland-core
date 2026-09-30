// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>
#include <erbsland/data/json/JsonValue.hpp>
#include <erbsland/text/base_n/BaseNDecoder.hpp>
#include <erbsland/text/base_n/BaseNEncoder.hpp>

namespace demo {

using namespace el::text::literals;

/// Encode one JSON object per LF-terminated TCP record and decode bounded incoming records.
/// @notest{The token authentication demo exercises this framing in both processes.}
class JsonLine final {
public:
    [[nodiscard]] static auto encode(const el::json::JsonValue &value) -> el::ByteBlock {
        auto bytes = el::ByteBlockEditor{
            el::StringEncoder{value.toString()}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject)};
        bytes.append(el::Byte{'\n'});
        return bytes;
    }

    [[nodiscard]] static auto base64(const el::ByteBlock &bytes) -> el::String {
        return el::text::base_n::BaseNEncoder{bytes, base64Format()}.toString();
    }

    [[nodiscard]] static auto unbase64(const el::String &text, const el::ByteLength maximum) -> el::ByteBlock {
        const auto result = el::text::base_n::BaseNDecoder{text, base64Format()}.toDataOrThrow(maximum);
        if (base64(result) != text) {
            throw el::ApplicationError{"Noncanonical Base64url in authentication record."_el};
        }
        return result;
    }

    template <typename Fn>
    void receive(const el::ByteBlock &data, Fn &&onLine) {
        for (const auto byte : data.span()) {
            if (byte == el::Byte{'\n'}) {
                if (_buffer.isEmpty()) {
                    throw el::ApplicationError{"Empty authentication record."_el};
                }
                const auto source = el::StringDecoder{_buffer}.decode(
                    el::StringEncoding::Utf8, el::StringBomMode::Reject, el::EncodingMode::Strict);
                _buffer.clear();
                auto limits = el::json::JsonParseOptions{};
                limits.setMaximumInputLength(el::ByteLength{4096U});
                const auto value = el::json::JsonValue::fromStringOrThrow(source, limits);
                if (!value.is(el::json::JsonType::Object)) {
                    throw el::ApplicationError{"Authentication record must be a JSON object."_el};
                }
                onLine(value);
            } else {
                if (_buffer.length() >= el::ByteLength{4096U} || byte == el::Byte{'\r'}) {
                    throw el::ApplicationError{"Authentication record is too long or contains CR."_el};
                }
                _buffer.append(byte);
            }
        }
    }

private:
    [[nodiscard]] static auto base64Format() -> el::text::base_n::BaseNFormat {
        auto format = el::text::base_n::BaseNFormat::base64Url();
        format.clearFlags(el::text::base_n::BaseNFormatFlag::All);
        format.setPadding(std::nullopt);
        format.setWhitespace(el::text::CharSet{});
        return format;
    }

private:
    el::ByteBlockEditor _buffer;
};

}
