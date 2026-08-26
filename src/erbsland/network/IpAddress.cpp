// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IpAddress.hpp"

#include "impl/host/CommonHostTests.hpp"

#include "../err/ParseError.hpp"
#include "../text/IntegerBase.hpp"
#include "../text/IntegerFormat.hpp"
#include "../text/IntegerParseOptions.hpp"
#include "../text/Literals.hpp"
#include "../text/ReadNumberStatus.hpp"
#include "../text/StringCharReader.hpp"
#include "../text/StringEditor.hpp"
#include "../unit/ByteIndex.hpp"
#include "../unit/CpLength.hpp"
#include "../util/HashHelper.hpp"

#include <algorithm>
#include <array>

namespace erbsland::network {

using namespace unit;
using namespace text;
using namespace text::literals;

auto IpAddress::formatV4(const Bytes &bytes, const std::size_t offset) -> String {
    auto result = StringEditor{};
    for (auto index = offset; index < offset + 4U; ++index) {
        if (index != offset) {
            result.append("."_el);
        }
        result.append(String::fromInteger(bytes.get(ByteIndex::fromSizeT(index)).toUInt8()));
    }
    return result;
}

auto IpAddress::isV4Mapped(const Bytes &bytes) noexcept -> bool {
    const auto span = bytes.span();
    return std::all_of(
               span.begin(), span.begin() + 10, [](const mem::Byte value) -> bool { return value == mem::Byte{}; }) &&
        bytes.get(ByteIndex{10U}) == mem::Byte{0xffU} && bytes.get(ByteIndex{11U}) == mem::Byte{0xffU};
}

auto IpAddress::formatV6(const Bytes &bytes) -> String {
    if (isV4Mapped(bytes)) {
        return String::fromJoined({"::ffff:"_el, formatV4(bytes, 12U)});
    }
    auto words = std::array<uint16_t, 8>{};
    for (auto index = std::size_t{0}; index < words.size(); ++index) {
        words[index] = static_cast<uint16_t>(
            (bytes.get(ByteIndex::fromSizeT(index * 2U)).toUInt16() << 8U) |
            bytes.get(ByteIndex::fromSizeT(index * 2U + 1U)).toUInt16());
    }
    auto bestStart = words.size();
    auto bestLength = std::size_t{0};
    for (auto start = std::size_t{0}; start < words.size();) {
        if (words[start] != 0U) {
            ++start;
            continue;
        }
        auto end = start;
        while (end < words.size() && words[end] == 0U) {
            ++end;
        }
        if (end - start > bestLength && end - start >= 2U) {
            bestStart = start;
            bestLength = end - start;
        }
        start = end;
    }
    auto result = StringEditor{};
    for (auto index = std::size_t{0}; index < words.size();) {
        if (index == bestStart) {
            result.append("::"_el);
            index += bestLength;
            continue;
        }
        if (index > 0U && index != bestStart + bestLength) {
            result.append(":"_el);
        }
        result.append(String::fromInteger(words[index], IntegerFormat::hexadecimal()));
        ++index;
    }
    return result;
}

auto IpAddress::parseV4(StringCharReader &reader, V4Bytes &bytes) noexcept -> bool {
    auto options = IntegerParseOptions{};
    options.setFixedBase(IntegerBase::Decimal).setMinimumDigits(CpLength::one()).setMaximumDigits(CpLength{3U});

    for (auto index = std::size_t{0}; index < 4U; ++index) {
        const auto firstCharacter = reader.peek();
        const auto value = reader.parseInteger(options);
        if (value.status != ReadNumberStatus::Success || value.value > 255U ||
            (value.digitCount > CpLength::one() && firstCharacter == U'0')) {
            return false;
        }
        bytes.set(ByteIndex::fromSizeT(index), mem::Byte::fromUInt8(static_cast<uint8_t>(value.value)));
        if (index < 3U) {
            if (!reader.advanceIf(U'.')) {
                return false;
            }
        } else if (!reader.isAtEnd()) {
            return false;
        }
    }
    return true;
}

auto IpAddress::parseV6(StringCharReader &reader, Bytes &bytes) noexcept -> bool {
    auto options = IntegerParseOptions{};
    options.setFixedBase(IntegerBase::Hexadecimal).setMinimumDigits(CpLength::one()).setMaximumDigits(CpLength{4U});

    auto words = std::array<uint16_t, 8>{};
    auto wordCount = std::size_t{};
    auto compressionIndex = std::optional<std::size_t>{};
    if (reader.advanceIf(U':')) {
        if (!reader.advanceIf(U':')) {
            return false;
        }
        compressionIndex = 0U;
    }

    while (!reader.isAtEnd()) {
        if (wordCount >= words.size()) {
            return false;
        }
        const auto wordState = reader.save();
        const auto word = reader.parseInteger(options);
        if (word.status != ReadNumberStatus::Success) {
            return false;
        }
        if (reader.peek() == U'.') {
            if (wordCount > 6U) {
                return false;
            }
            reader.restore(wordState);
            auto v4Bytes = V4Bytes{};
            if (!parseV4(reader, v4Bytes)) {
                return false;
            }
            words[wordCount++] = static_cast<uint16_t>(
                (v4Bytes.get(ByteIndex{0U}).toUInt16() << 8U) | v4Bytes.get(ByteIndex{1U}).toUInt16());
            words[wordCount++] = static_cast<uint16_t>(
                (v4Bytes.get(ByteIndex{2U}).toUInt16() << 8U) | v4Bytes.get(ByteIndex{3U}).toUInt16());
            break;
        }
        words[wordCount++] = static_cast<uint16_t>(word.value);
        if (reader.isAtEnd()) {
            break;
        }
        if (!reader.advanceIf(U':')) {
            return false;
        }
        if (reader.advanceIf(U':')) {
            if (compressionIndex.has_value()) {
                return false;
            }
            compressionIndex = wordCount;
        }
    }

    if ((!compressionIndex.has_value() && wordCount != words.size()) ||
        (compressionIndex.has_value() && wordCount >= words.size())) {
        return false;
    }
    const auto compressedWordCount = words.size() - wordCount;
    for (auto destination = std::size_t{}; destination < words.size(); ++destination) {
        auto word = uint16_t{};
        if (!compressionIndex.has_value() || destination < *compressionIndex) {
            word = words[destination];
        } else if (destination >= *compressionIndex + compressedWordCount) {
            word = words[destination - compressedWordCount];
        }
        bytes.set(ByteIndex::fromSizeT(destination * 2U), mem::Byte::fromCroppedUInt16(word >> 8U));
        bytes.set(ByteIndex::fromSizeT(destination * 2U + 1U), mem::Byte::fromCroppedUInt16(word));
    }
    return true;
}

auto IpAddress::isAny() const noexcept -> bool {
    const auto span = _bytes.span();
    const auto length = isV4() ? 4U : span.size();
    return std::all_of(span.begin(), span.begin() + static_cast<std::ptrdiff_t>(length), [](const mem::Byte value) {
        return value == mem::Byte{};
    });
}

auto IpAddress::isLoopback() const noexcept -> bool {
    if (isV4()) {
        return _bytes.get(ByteIndex::zero()) == mem::Byte{127U};
    }
    const auto span = _bytes.span();
    return std::all_of(span.begin(), span.end() - 1, [](const mem::Byte value) { return value == mem::Byte{}; }) &&
        _bytes.get(ByteIndex{15U}) == mem::Byte{1U};
}

auto IpAddress::toString() const -> String {
    return isV4() ? formatV4(_bytes) : formatV6(_bytes);
}

auto IpAddress::toHash() const noexcept -> std::size_t {
    auto result = util::createHash(_version);
    const auto length = isV4() ? 4U : _bytes.span().size();
    for (auto index = std::size_t{0}; index < length; ++index) {
        util::advanceHash(result, _bytes.get(ByteIndex::fromSizeT(index)).toUInt8());
    }
    return result;
}

auto IpAddress::fromString(const String &text) noexcept -> std::optional<IpAddress> {
    try {
        return fromStringOrThrow(text);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto IpAddress::fromStringOrThrow(const String &text) -> IpAddress {
    impl::testCommonHostText(text, "IP-address"_el);

    auto reader = StringCharReader{text};
    auto bytes = Bytes{};
    auto v4Bytes = V4Bytes{};
    if (parseV4(reader, v4Bytes)) {
        bytes.overwrite(v4Bytes.span());
        return IpAddress{IpVersion::V4, bytes};
    }
    reader.reset();
    if (parseV6(reader, bytes)) {
        return IpAddress{IpVersion::V6, bytes};
    }
    throw err::ParseError{
        text.contains(":"_el) ? "The IPv6 address syntax is invalid."_el : "The IPv4 address syntax is invalid."_el};
}

auto IpAddress::fromBytes(const IpVersion version, Bytes bytes) noexcept -> IpAddress {
    if (version == IpVersion::V4) {
        bytes.fill(ByteRange{ByteIndex{4U}, ByteLength::infinite()}, {});
    }
    return IpAddress{version, bytes};
}

auto IpAddress::anyV4() noexcept -> IpAddress {
    return {};
}

auto IpAddress::anyV6() noexcept -> IpAddress {
    return IpAddress{IpVersion::V6, {}};
}

auto IpAddress::loopbackV4() noexcept -> IpAddress {
    auto bytes = Bytes{};
    bytes.set(ByteIndex{0U}, mem::Byte{127U});
    bytes.set(ByteIndex{3U}, mem::Byte{1U});
    return IpAddress{IpVersion::V4, bytes};
}

auto IpAddress::loopbackV6() noexcept -> IpAddress {
    auto bytes = Bytes{};
    bytes.set(ByteIndex{15U}, mem::Byte{1U});
    return IpAddress{IpVersion::V6, bytes};
}

}
