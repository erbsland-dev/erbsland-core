// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Asn1Writer.hpp"

#include "Asn1ObjectIdentifierCodec.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../err/OutOfRangeFailure.hpp"
#include "../../../err/ParameterFailure.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::data::asn1::impl {

using namespace mem;
using namespace unit;
using namespace err;
using namespace text::literals;

void Asn1Writer::markAsSensitive() noexcept {
    // An empty editor has no allocation to carry the sensitive flag.
    _data.reserve(ByteCount::one());
    _data.markAsSensitive();
}

void Asn1Writer::requireCapacity(const ByteCount extra) const {
    if (extra > _limits.maximumLength - _data.length()) {
        throw OutOfRangeFailure{"ASN.1 writer byte limit exceeded."_el};
    }
}

void Asn1Writer::requireNode(const std::size_t depth, const ItemCount count) const {
    if (depth >= _frames.size() || depth >= _limits.maximumDepth.toRawValue() || count >= _limits.maximumNodes) {
        throw OutOfRangeFailure{"ASN.1 writer depth or node limit exceeded."_el};
    }
}

auto Asn1Writer::encoded() const -> ByteBlock {
    if (_depth != 0U) {
        throw LogicFailure{"ASN.1 output has open scopes."_el};
    }
    return _data;
}

auto Asn1Writer::begin(const Asn1Tag tag, const bool constructed, const Order order, const std::optional<Byte> prefix)
    -> Scope {
    requireNode(_depth, _nodes);
    auto identifier = ByteArray<6U>{};
    auto start = identifier.lastIndex();
    auto first = static_cast<uint8_t>(static_cast<unsigned>(tag.tagClass) << 6U);
    if (constructed) {
        first |= 0x20U;
    }
    if (tag.number < 31U) {
        identifier.set(start, Byte{static_cast<uint8_t>(first | tag.number)});
    } else {
        auto remaining = tag.number;
        identifier.set(start, Byte{static_cast<uint8_t>(remaining & 0x7fU)});
        while ((remaining >>= 7U) != 0U) {
            identifier.set(--start, Byte{static_cast<uint8_t>(0x80U | (remaining & 0x7fU))});
        }
        identifier.set(--start, Byte{static_cast<uint8_t>(first | 0x1fU)});
    }
    const auto size = start.absoluteDistanceTo(identifier.endIndex());
    requireCapacity(size + ByteCount::fromSizeT(1U + prefix.has_value()));
    auto &frame = _frames[_depth];
    frame = Frame{.id = ++_serial, .start = _data.endIndex(), .nodesBefore = _nodes, .tag = tag, .order = order};
    _data.append(identifier.span(start, size));
    frame.lengthIndex = _data.endIndex();
    _data.append(Byte{});
    if (prefix) {
        _data.append(*prefix);
    }
    ++_depth;
    ++_nodes;
    return Scope{this, frame.id};
}

auto Asn1Writer::top(const Scope scope) -> Frame & {
    if (scope._owner != this || _depth == 0U || _frames[_depth - 1U].id != scope._id) {
        throw LogicFailure{"ASN.1 scope belongs to a different opening."_el};
    }
    return _frames[_depth - 1U];
}

void Asn1Writer::cancel(const Scope scope) {
    const auto &frame = top(scope);
    _data.resize(frame.start.distanceFromZero());
    _nodes = frame.nodesBefore;
    --_depth;
}

void Asn1Writer::finishChild(const Asn1Tag tag, const ByteIndex start) {
    if (_depth == 0U) {
        return;
    }
    auto &parent = _frames[_depth - 1U];
    if (parent.order == Order::Insertion) {
        return;
    }
    if (parent.previous) {
        if (parent.order == Order::Tag && parent.previousTag >= tag) {
            throw ParameterFailure{"DER SET components must have ascending distinct tags."_el, "value"_el};
        }
        if (parent.order == Order::Encoding &&
            _data.slice(*parent.previous, start) > _data.slice(start, _data.endIndex())) {
            throw ParameterFailure{"DER SET OF components must be ordered by encoding."_el, "value"_el};
        }
    }
    parent.previous = start;
    parent.previousTag = tag;
}

void Asn1Writer::end(const Scope scope) {
    const auto &frame = top(scope);
    auto length = frame.lengthIndex.incremented().absoluteDistanceTo(_data.endIndex()).toRawValue();
    if (length < 128U) {
        _data.set(frame.lengthIndex, Byte{static_cast<uint8_t>(length)});
    } else {
        auto bytes = ByteArray<sizeof(length)>{};
        auto first = bytes.endIndex();
        while (length != 0U) {
            bytes.set(--first, Byte{static_cast<uint8_t>(length)});
            length >>= 8U;
        }
        const auto count = first.absoluteDistanceTo(bytes.endIndex());
        if (count > _limits.maximumLength - _data.length()) {
            cancel(scope);
            throw OutOfRangeFailure{"ASN.1 length field exceeds the byte budget."_el};
        }
        _data.set(frame.lengthIndex, Byte{static_cast<uint8_t>(0x80U | count.toRawValue())});
        _data.insert(frame.lengthIndex.incremented(), bytes.span(first, count));
    }
    --_depth;
    try {
        finishChild(frame.tag, frame.start);
    } catch (const ParameterFailure &) {
        _data.resize(frame.start.distanceFromZero());
        _nodes = frame.nodesBefore;
        throw;
    }
}

void Asn1Writer::primitive(const Asn1Tag tag, const ConstByteSpan content, const std::optional<Byte> prefix) {
    const auto scope = begin(tag, false);
    auto size = ByteCount::fromSizeT(content.size());
    if (prefix) {
        ++size;
    }
    if (size > _limits.maximumLength - _data.length()) {
        cancel(scope);
        throw OutOfRangeFailure{"ASN.1 primitive exceeds the byte budget."_el};
    }
    if (prefix) {
        _data.append(*prefix);
    }
    _data.append(content);
    end(scope);
}

void Asn1Writer::integer(const Asn1Tag tag, const int64_t value) {
    auto bytes = ByteArray<sizeof(value)>{};
    auto first = bytes.endIndex();
    auto remaining = value;
    do {
        const auto octet = static_cast<uint8_t>(remaining);
        bytes.set(--first, Byte{octet});
        remaining >>= 8U;
        if ((remaining == 0 && (octet & 0x80U) == 0U) || (remaining == -1 && (octet & 0x80U) != 0U)) {
            break;
        }
    } while (!first.isZero());
    primitive(tag, bytes.span(first, first.absoluteDistanceTo(bytes.endIndex())));
}

void Asn1Writer::accountTree(const Asn1Node &node, const std::size_t depth, ItemCount &count) const {
    requireNode(depth, count);
    ++count;
    for (const auto &child : node.children()) {
        accountTree(child, depth + 1U, count);
    }
}

void Asn1Writer::appendDer(const Asn1Node &node) {
    if (!node.isDer()) {
        throw ParameterFailure{"Embedding requires a DER-validated node."_el, "value"_el};
    }
    const auto bytes = node.encodedData();
    requireCapacity(bytes.length());
    auto count = _nodes;
    accountTree(node, _depth, count);
    const auto start = _data.endIndex();
    _data.append(bytes.span());
    try {
        finishChild(node.tag(), start);
    } catch (const ParameterFailure &) {
        _data.resize(start.distanceFromZero());
        throw;
    }
    _nodes = count;
}

void Asn1Writer::objectIdentifier(const Asn1ObjectIdentifier &oid) {
    if (oid.isEmpty()) {
        throw ParameterFailure{"Expected a nonempty ASN.1 object identifier."_el, "value"_el};
    }
    const auto scope = begin(Asn1UniversalType::ObjectIdentifier, false);
    try {
        Asn1ObjectIdentifierCodec::encode(oid, _data, _limits.maximumLength);
    } catch (const OutOfRangeFailure &) {
        cancel(scope);
        throw;
    }
    end(scope);
}

}
