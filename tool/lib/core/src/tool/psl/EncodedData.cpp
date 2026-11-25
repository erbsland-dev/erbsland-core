// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncodedData.hpp"

#include <erbsland/BitWriter.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/U8StringConstIterator.hpp>

#include <algorithm>
#include <limits>

namespace tool::psl {

using namespace el::text::literals;

EncodedData::EncodedData(const el::List<Rule> &rules) : _rules{rules} {
    if (_rules.count().toSizeT() > std::numeric_limits<std::uint32_t>::max()) {
        throw el::ParseError{"The PSL rule count exceeds the runtime representation."_el};
    }
    if (_rules.isEmpty()) {
        throw el::ParseError{"An empty Public Suffix List cannot be encoded."_el};
    }
    auto prefixFrequencies = Frequencies{};
    auto contentFrequencies = Frequencies{};
    const auto prefixes = collectFrequencies(prefixFrequencies, contentFrequencies);
    const auto prefixEncoder = HuffmanTree{prefixFrequencies};
    const auto contentEncoder = HuffmanTree{contentFrequencies};
    _prefixTree = prefixEncoder.flattened();
    _contentTree = contentEncoder.flattened();
    encode(prefixes, prefixEncoder, contentEncoder);
    if (storageByteCount() > el::ByteCount{45000}) {
        throw el::ParseError{"The PSL payload exceeds the runtime storage budget."_el};
    }
    verify();
}

void EncodedData::validateRule(const Rule &rule, const el::String &previous) {
    if (rule.token > 2U || rule.text.isEmpty() || (!previous.isEmpty() && rule.text <= previous)) {
        throw el::ParseError{"Rules must be nonempty, sorted, unique, and use a known terminal token."_el};
    }
    const auto length = rule.text.length();
    if (length >= el::ByteCount{256}) {
        throw el::ParseError{"A rule exceeds the runtime reconstruction buffer."_el};
    }
    _maximumRuleLength = std::max(_maximumRuleLength, length);
    auto labelCount = 1U;
    for (const auto character : rule.text) {
        if (character == U'.') {
            ++labelCount;
        }
    }
    _maximumLabelCount = std::max(_maximumLabelCount, labelCount);
    _originalTextByteCount += length + el::ByteCount::one();
}

auto EncodedData::collectFrequencies(Frequencies &prefixFrequencies, Frequencies &contentFrequencies)
    -> el::List<el::ByteCount> {
    auto prefixes = el::List<el::ByteCount>{};
    auto previous = el::String{};
    auto index = el::ItemIndex::zero();
    for (const auto &rule : _rules) {
        validateRule(rule, previous);
        auto prefix = el::ByteCount{};
        if (index.toRawValue() % cBlockSize != 0U) {
            prefix = prefixLength(previous, rule.text);
        }
        if (prefix >= el::ByteCount{128}) {
            throw el::ParseError{"A common prefix exceeds the Huffman token representation."_el};
        }
        prefixes.append(prefix);
        ++prefixFrequencies[prefix.toSizeT()];
        for (const auto character : rule.text.slice(el::StringSide::Back, el::ByteIndex::end(prefix))) {
            ++contentFrequencies[characterToken(character)];
        }
        ++contentFrequencies[rule.token];
        previous = rule.text;
        ++index;
    }
    return prefixes;
}

void EncodedData::encode(
    const el::List<el::ByteCount> &prefixes, const HuffmanTree &prefixEncoder, const HuffmanTree &contentEncoder) {
    auto writer = el::BitWriter{};
    auto index = el::ItemIndex::zero();
    for (const auto &rule : _rules) {
        if (index.toRawValue() % cBlockSize == 0U) {
            _blockOffsets.append(static_cast<std::uint32_t>(writer.bitCount()));
        }
        const auto prefix = prefixes.get(index);
        prefixEncoder.write(writer, static_cast<unsigned>(prefix.toSizeT()));
        for (const auto character : rule.text.slice(el::StringSide::Back, el::ByteIndex::end(prefix))) {
            contentEncoder.write(writer, characterToken(character));
        }
        contentEncoder.write(writer, rule.token);
        if (writer.bitCount() > std::numeric_limits<std::uint32_t>::max()) {
            throw el::ParseError{"PSL bit offsets overflow."_el};
        }
        ++index;
    }
    _blockOffsets.append(static_cast<std::uint32_t>(writer.bitCount()));
    _encodedRules = writer.toByteBlock();
}

auto EncodedData::prefixLength(const el::String &a, const el::String &b) -> el::ByteCount {
    const auto length = std::min(a.length(), b.length());
    auto index = el::ByteIndex{};
    auto prefix = el::ByteCount{};
    while (index.isWithin(length)) {
        const auto character = a.charAt(index);
        if (character != b.readCharAndAdvance(index)) {
            break;
        }
        prefix = index.distanceFromZero();
    }
    return prefix;
}

auto EncodedData::alphabet() -> const el::String & {
    static const auto value = el::String{"-.0123456789abcdefghijklmnopqrstuvwxyz"_el};
    return value;
}

auto EncodedData::characterToken(const el::Char character) -> unsigned {
    const auto index = alphabet().findFirstOf(el::CharSet(character));
    if (!index.isValid()) {
        throw el::ParseError{"Unsupported character in an ASCII PSL rule."_el};
    }
    return static_cast<unsigned>(index.toSizeT()) + 3U;
}

auto EncodedData::storageByteCount() const -> el::ByteCount {
    return _encodedRules.length() +
        el::ByteCount::fromSizeTOrThrow(_blockOffsets.count().toSizeT() * sizeof(std::uint32_t)) +
        _prefixTree.length() + _contentTree.length();
}

auto EncodedData::decodeToken(el::BitReader &reader, const std::uint32_t end, const el::ByteBlock &tree) -> unsigned {
    auto node = 0U;
    for (auto depth = 0U; depth < 128U; ++depth) {
        if (reader.bitPosition() >= end || node * 2U + 1U >= tree.length().toSizeT()) {
            throw el::ParseError{"Invalid or truncated PSL Huffman stream."_el};
        }
        const auto bit = reader.readBoolOrThrow();
        const auto entry = tree.span()[node * 2U + static_cast<unsigned>(bit)].toUInt8();
        if ((entry & 0x80U) != 0U) {
            return entry & 0x7FU;
        }
        node = entry;
    }
    throw el::ParseError{"Cyclic PSL Huffman tree."_el};
}

auto EncodedData::decodeRule(el::BitReader &reader, const std::uint32_t end, const el::String &previous) const -> Rule {
    const auto prefix = el::ByteCount::fromInteger(decodeToken(reader, end, _prefixTree));
    if (prefix > previous.length()) {
        throw el::ParseError{"A decoded prefix exceeds its preceding rule."_el};
    }
    auto text = el::StringEditor{previous.slice(el::StringSide::Front, prefix)};
    auto token = decodeToken(reader, end, _contentTree);
    while (token > 2U) {
        if (token - 3U >= alphabet().length().toSizeT() || text.length() >= _maximumRuleLength) {
            throw el::ParseError{"Invalid PSL content symbol or rule length."_el};
        }
        text.append(alphabet().charAt(el::ByteIndex::fromInteger(token - 3U)));
        token = decodeToken(reader, end, _contentTree);
    }
    return Rule{text, token};
}

void EncodedData::verifyBlock(const el::ItemIndex block, el::ItemIndex &index) const {
    const auto end = _blockOffsets.get(block.incremented());
    auto reader = el::BitReader{_encodedRules};
    reader.setBitPosition(_blockOffsets.get(block));
    auto previous = el::String{};
    const auto last = std::min(el::ItemCount::fromInteger((block.toRawValue() + 1U) * cBlockSize), _rules.count());
    while (index.isWithin(last)) {
        const auto decoded = decodeRule(reader, end, previous);
        const auto &expected = _rules.get(index);
        if (decoded != expected) {
            throw el::ParseError{"The PSL payload failed its exact round trip."_el};
        }
        previous = decoded.text;
        index.uncheckedIncrement();
    }
    if (reader.bitPosition() != end) {
        throw el::ParseError{"A PSL block has an incorrect end offset."_el};
    }
}

void EncodedData::verify() const {
    auto index = el::ItemIndex::zero();
    for (
        auto block = el::ItemIndex::zero(); block.isWithin(_blockOffsets.count() - el::ItemCount::one());
        block.uncheckedIncrement()) {
        verifyBlock(block, index);
    }
    if (index != el::ItemIndex::end(_rules.count())) {
        throw el::ParseError{"The PSL payload has incomplete block offsets."_el};
    }
}

}
