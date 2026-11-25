// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1Node.hpp"

#include "impl/Asn1ObjectIdentifierCodec.hpp"
#include "impl/DecodedNode.hpp"
#include "impl/PrimitiveContent.hpp"

#include "../../err/ParseError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/EncodingMode.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringDecoder.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringEncoding.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ByteIndex.hpp"

namespace erbsland::data::asn1 {

using namespace mem;
using namespace text;
using namespace unit;

Asn1Node::Asn1Node(impl::DecodedNode &&value) noexcept :
    _encodedData{std::move(value.encoded)},
    _offset{value.offset},
    _headerLength{value.headerLength},
    _tag{value.tag},
    _constructed{value.constructed},
    _der{value.der},
    _children{std::move(value.children)} {
}

auto Asn1Node::matches(const Asn1Tag expected, const bool constructed) const noexcept -> bool {
    return !isEmpty() && _tag == expected && _constructed == constructed;
}

auto Asn1Node::matchesPrimitive(const Asn1Tag expected, const Asn1UniversalType type) const noexcept -> bool {
    return matches(expected, false) && (expected.tagClass != Asn1TagClass::Universal || expected == Asn1Tag{type});
}

auto Asn1Node::isNull(const Asn1Tag expected) const noexcept -> bool {
    return matchesPrimitive(expected, Asn1UniversalType::Null) && _encodedData.length() == _headerLength;
}

auto Asn1Node::isEmpty() const noexcept -> bool {
    return _encodedData.isEmpty();
}

auto Asn1Node::isConstructed() const noexcept -> bool {
    return !isEmpty() && _constructed;
}

auto Asn1Node::tagClass() const noexcept -> Asn1TagClass {
    return _tag.tagClass;
}

auto Asn1Node::tagNumber() const noexcept -> uint32_t {
    return _tag.number;
}

auto Asn1Node::universalType() const noexcept -> Asn1UniversalType {
    if (isEmpty() || tagClass() != Asn1TagClass::Universal) {
        return Asn1UniversalType::None;
    }
    return static_cast<Asn1UniversalType>(tagNumber());
}

auto Asn1Node::childCount() const noexcept -> ItemCount {
    return _children.count();
}

auto Asn1Node::child(const ItemIndex index) const noexcept -> Asn1Node {
    if (isEmpty() || !index.isWithin(_children.count())) {
        return {};
    }
    return _children.getRefOrThrow(index);
}

auto Asn1Node::children() const -> util::List<Asn1Node> {
    return _children;
}

auto Asn1Node::encodedData() const -> ByteBlock {
    return _encodedData;
}

auto Asn1Node::contentData() const -> ByteBlock {
    if (isEmpty()) {
        return {};
    }
    return _encodedData.slice(ByteIndex::end(_headerLength), _encodedData.length() - _headerLength);
}

auto Asn1Node::toBoolean(const Asn1Tag expected) const noexcept -> std::optional<bool> {
    if (!matchesPrimitive(expected, Asn1UniversalType::Boolean)) {
        return std::nullopt;
    }
    const auto content = contentData();
    if (content.length() != ByteCount{1U}) {
        return std::nullopt;
    }
    const auto byte = content.span().front().toUInt8();
    if (byte != 0U && byte != 0xffU) {
        return std::nullopt;
    }
    return byte != 0U;
}

auto Asn1Node::toObjectIdentifier(const Asn1Tag expected) const noexcept -> std::optional<Asn1ObjectIdentifier> {
    try {
        return toObjectIdentifierOrThrow(expected);
    } catch (const err::ParseError &) {
        return std::nullopt;
    }
}

auto Asn1Node::toObjectIdentifierOrThrow(const Asn1Tag expected) const -> Asn1ObjectIdentifier {
    if (!matchesPrimitive(expected, Asn1UniversalType::ObjectIdentifier)) {
        throw err::ParseError{"Expected a primitive object identifier.", encodedByteIndex()};
    }
    return Asn1ObjectIdentifier{impl::Asn1ObjectIdentifierCodec{contentData(), contentByteIndex()}.decode()};
}

auto Asn1Node::toInteger(const Asn1Tag expected) const noexcept -> std::optional<int64_t> {
    return signedInteger(expected, Asn1UniversalType::Integer);
}

auto Asn1Node::signedInteger(const Asn1Tag expected, const Asn1UniversalType type) const noexcept
    -> std::optional<int64_t> {
    if (!matchesPrimitive(expected, type)) {
        return std::nullopt;
    }
    const auto content = contentData();
    const auto bytes = content.span();
    if ((expected.tagClass != Asn1TagClass::Universal && !impl::isMinimalInteger(bytes)) ||
        bytes.size() > sizeof(int64_t)) {
        return std::nullopt;
    }
    uint64_t bits{};
    if ((bytes.front().toUInt8() & 0x80U) != 0U) {
        bits = UINT64_MAX;
    }
    for (const auto byte : bytes) {
        bits = (bits << 8U) | byte.toUInt8();
    }
    return static_cast<int64_t>(bits);
}

auto Asn1Node::toUnsignedInteger(const Asn1Tag expected) const noexcept -> std::optional<uint64_t> {
    const auto magnitude = toUnsignedMagnitude(expected);
    if (!magnitude) {
        return std::nullopt;
    }
    const auto bytes = magnitude->span();
    if (bytes.size() > sizeof(uint64_t)) {
        return std::nullopt;
    }
    auto result = uint64_t{};
    for (const auto byte : bytes) {
        result = (result << 8U) | byte.toUInt8();
    }
    return result;
}

auto Asn1Node::toUnsignedMagnitude(const Asn1Tag expected) const noexcept -> std::optional<ByteBlock> {
    if (!matchesPrimitive(expected, Asn1UniversalType::Integer)) {
        return std::nullopt;
    }
    const auto content = contentData();
    const auto bytes = content.span();
    if ((expected.tagClass != Asn1TagClass::Universal && !impl::isMinimalInteger(bytes)) ||
        (bytes.front().toUInt8() & 0x80U) != 0U) {
        return std::nullopt;
    }
    if (bytes.front().toUInt8() == 0U) {
        return content.slice(ByteIndex::one(), content.endIndex());
    }
    return content;
}

auto Asn1Node::toEnumerated(const Asn1Tag expected) const noexcept -> std::optional<int64_t> {
    return signedInteger(expected, Asn1UniversalType::Enumerated);
}

auto Asn1Node::toOctetString(const Asn1Tag expected) const noexcept -> std::optional<ByteBlock> {
    if (matchesPrimitive(expected, Asn1UniversalType::OctetString)) {
        return contentData();
    }
    return std::nullopt;
}

auto Asn1Node::toBitString(const Asn1Tag expected) const noexcept -> std::optional<Asn1BitString> {
    if (!matchesPrimitive(expected, Asn1UniversalType::BitString)) {
        return std::nullopt;
    }
    const auto content = contentData();
    if (expected.tagClass != Asn1TagClass::Universal && !impl::isValidBitString(content.span())) {
        return std::nullopt;
    }
    return Asn1BitString{content.slice(ByteIndex::one(), content.endIndex()), content.span().front().toUInt8()};
}

auto Asn1Node::toString() const noexcept -> std::optional<String> {
    try {
        const auto type = universalType();
        const auto content = contentData();
        if (type == Asn1UniversalType::Utf8String) {
            return StringDecoder{content}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
        }
        auto result = StringEditor{};
        if (type == Asn1UniversalType::BmpString) {
            if (content.span().size() % 2U != 0U) {
                return std::nullopt;
            }
            for (auto index = std::size_t{}; index < content.span().size(); index += 2U) {
                const auto codePoint = static_cast<char32_t>(
                    (content.span()[index].toUInt32() << 8U) | content.span()[index + 1U].toUInt32());
                const auto character = Char{codePoint};
                if (!character.isValidUnicode()) {
                    return std::nullopt;
                }
                result.append(character);
            }
            return result;
        }
        if (type == Asn1UniversalType::UniversalString) {
            if (content.span().size() % 4U != 0U) {
                return std::nullopt;
            }
            for (auto index = std::size_t{}; index < content.span().size(); index += 4U) {
                const auto codePoint = static_cast<char32_t>(
                    (content.span()[index].toUInt32() << 24U) | (content.span()[index + 1U].toUInt32() << 16U) |
                    (content.span()[index + 2U].toUInt32() << 8U) | content.span()[index + 3U].toUInt32());
                const auto character = Char{codePoint};
                if (!character.isValidUnicode()) {
                    return std::nullopt;
                }
                result.append(character);
            }
            return result;
        }
        const auto isAsciiType = type == Asn1UniversalType::NumericString ||
            type == Asn1UniversalType::PrintableString || type == Asn1UniversalType::TeletexString ||
            type == Asn1UniversalType::Ia5String || type == Asn1UniversalType::UtcTime ||
            type == Asn1UniversalType::GeneralizedTime;
        if (!isAsciiType) {
            return std::nullopt;
        }
        for (const auto byte : content.span()) {
            if (byte.toUInt8() > 0x7FU) {
                return std::nullopt;
            }
            result.append(Char{byte.toUInt32()});
        }
        return result;
    } catch (const err::RuntimeError &) {
        return std::nullopt;
    }
}

}
