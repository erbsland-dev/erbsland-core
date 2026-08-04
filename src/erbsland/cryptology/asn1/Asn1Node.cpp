// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1Node.hpp"

#include "../impl/Asn1ObjectIdentifierCodec.hpp"

#include "../../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../../text/EncodingMode.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringDecoder.hpp"
#include "../../text/StringEditor.hpp"
#include "../../text/StringEncoding.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::cryptology {

auto Asn1Node::isEmpty() const noexcept -> bool {
    return _encodedData.isEmpty();
}

auto Asn1Node::isConstructed() const noexcept -> bool {
    return !isEmpty() && (_encodedData.get(unit::ByteIndex::zero()).toUInt8() & 0x20U) != 0U;
}

auto Asn1Node::tagClass() const noexcept -> Asn1TagClass {
    if (isEmpty()) {
        return Asn1TagClass::Universal;
    }
    return static_cast<Asn1TagClass>((_encodedData.get(unit::ByteIndex::zero()).toUInt8() >> 6U) & 0x03U);
}

auto Asn1Node::tagNumber() const noexcept -> uint32_t {
    return isEmpty() ? 0U : _tagNumber;
}

auto Asn1Node::universalType() const noexcept -> Asn1UniversalType {
    if (isEmpty() || tagClass() != Asn1TagClass::Universal) {
        return Asn1UniversalType::None;
    }
    return static_cast<Asn1UniversalType>(tagNumber());
}

auto Asn1Node::childCount() const noexcept -> unit::ItemCount {
    return _children.count();
}

auto Asn1Node::child(const unit::ItemIndex index) const noexcept -> Asn1Node {
    if (isEmpty() || !index.isWithin(_children.count())) {
        return {};
    }
    return _children.getRefOrThrow(index);
}

auto Asn1Node::children() const -> util::List<Asn1Node> {
    return _children;
}

auto Asn1Node::encodedData() const -> mem::ByteBlock {
    return _encodedData;
}

auto Asn1Node::contentData() const -> mem::ByteBlock {
    if (isEmpty()) {
        return {};
    }
    return _encodedData.slice(unit::ByteIndex::end(_headerLength), _encodedData.length() - _headerLength);
}

auto Asn1Node::toBoolean() const noexcept -> std::optional<bool> {
    if (universalType() != Asn1UniversalType::Boolean) {
        return std::nullopt;
    }
    const auto content = contentData();
    if (content.length() != unit::ByteLength{1U}) {
        return std::nullopt;
    }
    return content.span().front().toUInt8() != 0U;
}

auto Asn1Node::toObjectIdentifier() const noexcept -> std::optional<Asn1ObjectIdentifier> {
    if (universalType() != Asn1UniversalType::ObjectIdentifier) {
        return std::nullopt;
    }
    try {
        return Asn1ObjectIdentifier{impl::Asn1ObjectIdentifierCodec{contentData(), unit::ByteIndex::zero()}.decode()};
    } catch (...) {
        return std::nullopt;
    }
}

auto Asn1Node::toString() const noexcept -> std::optional<text::String> {
    try {
        const auto type = universalType();
        const auto content = contentData();
        if (type == Asn1UniversalType::Utf8String) {
            return text::StringDecoder{content}.decode(
                text::StringEncoding::Utf8, text::StringBomMode::Reject, text::EncodingMode::Strict);
        }
        auto result = text::StringEditor{};
        if (type == Asn1UniversalType::BmpString) {
            if (content.span().size() % 2U != 0U) {
                return std::nullopt;
            }
            for (auto index = std::size_t{}; index < content.span().size(); index += 2U) {
                const auto codePoint = static_cast<char32_t>(
                    (content.span()[index].toUInt32() << 8U) | content.span()[index + 1U].toUInt32());
                const auto character = text::Char{codePoint};
                if (!character.isValidUnicode()) {
                    return std::nullopt;
                }
                result.append(character);
            }
            return text::String{result};
        }
        if (type == Asn1UniversalType::UniversalString) {
            if (content.span().size() % 4U != 0U) {
                return std::nullopt;
            }
            for (auto index = std::size_t{}; index < content.span().size(); index += 4U) {
                const auto codePoint = static_cast<char32_t>(
                    (content.span()[index].toUInt32() << 24U) | (content.span()[index + 1U].toUInt32() << 16U) |
                    (content.span()[index + 2U].toUInt32() << 8U) | content.span()[index + 3U].toUInt32());
                const auto character = text::Char{codePoint};
                if (!character.isValidUnicode()) {
                    return std::nullopt;
                }
                result.append(character);
            }
            return text::String{result};
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
            result.append(text::Char{byte.toUInt32()});
        }
        return text::String{result};
    } catch (...) {
        return std::nullopt;
    }
}

auto Asn1Node::encodedStorageByteIndex() const noexcept -> unit::ByteIndex {
    if (isEmpty()) {
        return unit::ByteIndex::zero();
    }
    return mem::impl::UnsafeByteBlockAccess{_encodedData}.dataView().range().index();
}

}
