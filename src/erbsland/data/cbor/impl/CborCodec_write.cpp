// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CborCodec.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringConverter.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <vector>

namespace erbsland::data::cbor::impl {

using namespace text::literals;

void CborCodec::writeHead(uint8_t major, uint64_t value) {
    const auto base = static_cast<uint8_t>(major << 5U);
    if (value < 24U) {
        _writer.writeUInt8(static_cast<uint8_t>(base | value));
    } else if (value <= UINT8_MAX) {
        _writer.writeUInt8(static_cast<uint8_t>(base | 24U));
        _writer.writeUInt8(static_cast<uint8_t>(value));
    } else if (value <= UINT16_MAX) {
        _writer.writeUInt8(static_cast<uint8_t>(base | 25U));
        _writer.writeUInt16(static_cast<uint16_t>(value));
    } else if (value <= UINT32_MAX) {
        _writer.writeUInt8(static_cast<uint8_t>(base | 26U));
        _writer.writeUInt32(static_cast<uint32_t>(value));
    } else {
        _writer.writeUInt8(static_cast<uint8_t>(base | 27U));
        _writer.writeUInt64(value);
    }
}

void CborCodec::writeBytes(const mem::ByteBlock &value) {
    writeHead(2U, value.length().toRawValue());
    _writer.writeBytes(value);
}

void CborCodec::writeText(const text::String &value) {
    if (_formatOptions.isDagCbor() && !value.isValidUtf8()) {
        throw err::ParameterError{"DAG-CBOR requires valid UTF-8."_el, "value"_el};
    }
    const auto bytes = text::StringConverter{value}.toStdString();
    writeHead(3U, bytes.size());
    _writer.writeBytes(std::span<const std::byte>{reinterpret_cast<const std::byte *>(bytes.data()), bytes.size()});
}

void CborCodec::writeValue(const CborValue &value) {
    switch (value.type()) {
    case CborType::Null:
        _writer.writeUInt8(0xf6U);
        break;
    case CborType::Bool:
        _writer.writeUInt8(value.getBool().value() ? 0xf5U : 0xf4U);
        break;
    case CborType::Signed: {
        const auto number = value.getSigned().value();
        if (number >= 0) {
            writeHead(0U, static_cast<uint64_t>(number));
        } else {
            writeHead(1U, static_cast<uint64_t>(-(number + 1)));
        }
        break;
    }
    case CborType::Unsigned:
        writeHead(0U, value.getUnsigned().value());
        break;
    case CborType::Float: {
        auto number = value.getFloat().value();
        if (_formatOptions.isDagCbor() && !std::isfinite(number)) {
            throw err::ParameterError{"DAG-CBOR requires a finite float."_el, "value"_el};
        }
        if (_formatOptions.isDagCbor() && number == 0.0) {
            number = 0.0;
        }
        _writer.writeUInt8(0xfbU);
        _writer.writeUInt64(std::bit_cast<uint64_t>(number));
        break;
    }
    case CborType::Text:
        writeText(value.getText().value());
        break;
    case CborType::Bytes:
        writeBytes(value.getBytes().value());
        break;
    case CborType::Array: {
        const auto array = value.getArray().value();
        writeHead(4U, array.count().toRawValue());
        for (const auto &entry : array) {
            writeValue(entry);
        }
        break;
    }
    case CborType::Map: {
        const auto map = value.getMap().value();
        writeHead(5U, map.count().toRawValue());
        if (_formatOptions.isDagCbor()) {
            auto entries = std::vector<std::pair<const text::String *, const CborValue *>>{};
            entries.reserve(map.count().toSizeT());
            for (const auto &[key, entry] : map) {
                entries.emplace_back(&key, &entry);
            }
            std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) -> bool {
                if (a.first->length() != b.first->length()) {
                    return a.first->length() < b.first->length();
                }
                return *a.first < *b.first;
            });
            for (const auto &[key, entry] : entries) {
                writeText(*key);
                writeValue(*entry);
            }
        } else {
            for (const auto &[key, entry] : map) {
                writeText(key);
                writeValue(entry);
            }
        }
        break;
    }
    case CborType::DateTime: {
        if (_formatOptions.isDagCbor()) {
            throw err::ParameterError{"DAG-CBOR does not support date/time tags."_el, "value"_el};
        }
        const auto date = value.getTimestamp().value();
        if (!date.isValid()) {
            throw err::ParameterError{"Invalid CBOR date/time."_el, "value"_el};
        }
        writeHead(6U, 0U);
        writeText(
            date.toDateTimeOrThrow().toIsoString(time::cDefaultDateTimeFormat, time::DateTimePrecision::Nanosecond));
        break;
    }
    case CborType::Link: {
        const auto cid = value.getLinkBytes().value();
        // Validate the complete tag payload before writing a link supplied by the caller.
        if (!validCid(cid)) {
            throw err::ParameterError{"A CID link must contain a valid binary CID."_el, "value"_el};
        }
        writeHead(6U, 42U);
        writeHead(2U, cid.length().toRawValue() + 1U);
        _writer.writeUInt8(0U);
        _writer.writeBytes(cid);
        break;
    }
    }
}

}
