// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PublicSuffixLookup.hpp"

#include "../../../../mem/ByteSpan.hpp"
#include "../../../../text/impl/UnsafeU8StringAccess.hpp"

#include <algorithm>

namespace erbsland::network::impl {

PublicSuffixLookup::PublicSuffixLookup() noexcept : _data{public_suffix_data::data()} {
}

auto PublicSuffixLookup::decodePrefixToken(mem::BitReader &reader) const -> std::size_t {
    auto node = std::size_t{};
    while (true) {
        const auto entry = _data.prefixTree[node * 2U + reader.readInteger<std::size_t>()];
        if ((entry & cLeafMask) != 0U) {
            return entry & (cLeafMask - 1U);
        }
        node = entry;
    }
}

auto PublicSuffixLookup::decodeContentToken(mem::BitReader &reader) const -> std::uint8_t {
    auto node = std::size_t{};
    while (true) {
        const auto entry = _data.contentTree[node * 2U + reader.readInteger<std::size_t>()];
        if ((entry & cLeafMask) != 0U) {
            return entry & (cLeafMask - 1U);
        }
        node = entry;
    }
}

auto PublicSuffixLookup::decodeRule(mem::BitReader &reader, RuleBuffer &rule) const -> bool {
    const auto prefixLength = decodePrefixToken(reader);
    if (prefixLength > rule.length || prefixLength > _data.maximumRuleLength) {
        return false;
    }
    rule.length = prefixLength;
    while (true) {
        const auto token = decodeContentToken(reader);
        if (token <= static_cast<std::uint8_t>(RuleType::Exception)) {
            rule.type = static_cast<RuleType>(token);
            return true;
        }
        if (rule.length >= rule.characters.size()) {
            return false;
        }
        const auto character = characterFromToken(token);
        if (character == '\0') {
            return false;
        }
        rule.characters[rule.length] = character;
        ++rule.length;
    }
}

auto PublicSuffixLookup::compare(const RuleBuffer &rule, const std::span<const char> key) noexcept
    -> std::strong_ordering {
    const auto commonLength = std::min(rule.length, key.size());
    for (auto index = std::size_t{}; index < commonLength; ++index) {
        if (rule.characters[index] < key[index]) {
            return std::strong_ordering::less;
        }
        if (rule.characters[index] > key[index]) {
            return std::strong_ordering::greater;
        }
    }
    return rule.length <=> key.size();
}

auto PublicSuffixLookup::findRule(const std::span<const char> key) const -> RuleType {
    if (key.empty() || _data.blockBitOffsets.size() < 2U) {
        return RuleType::None;
    }
    const auto blockCount = _data.blockBitOffsets.size() - 1U;
    auto lowerBlock = std::size_t{};
    auto upperBlock = blockCount;
    while (lowerBlock < upperBlock) {
        const auto block = lowerBlock + (upperBlock - lowerBlock) / 2U;
        auto reader = mem::BitReader{mem::toConstByteSpan(_data.encodedRules), _data.blockBitOffsets[block]};
        auto rule = RuleBuffer{};
        if (!decodeRule(reader, rule)) {
            return RuleType::None;
        }
        if (compare(rule, key) != std::strong_ordering::greater) {
            lowerBlock = block + 1U;
        } else {
            upperBlock = block;
        }
    }
    if (lowerBlock == 0U) {
        return RuleType::None;
    }
    const auto block = lowerBlock - 1U;
    auto reader = mem::BitReader{mem::toConstByteSpan(_data.encodedRules), _data.blockBitOffsets[block]};
    auto rule = RuleBuffer{};
    const auto firstRule = block * _data.blockSize;
    const auto lastRule = std::min(firstRule + _data.blockSize, _data.ruleCount);
    for (auto ruleIndex = firstRule; ruleIndex < lastRule; ++ruleIndex) {
        if (!decodeRule(reader, rule)) {
            return RuleType::None;
        }
        const auto order = compare(rule, key);
        if (order == std::strong_ordering::equal) {
            return rule.type;
        }
        if (order == std::strong_ordering::greater) {
            break;
        }
    }
    return RuleType::None;
}

auto PublicSuffixLookup::characterFromToken(const std::uint8_t token) noexcept -> char {
    if (token == 3U) {
        return '-';
    }
    if (token == 4U) {
        return '.';
    }
    if (token >= 5U && token <= 14U) {
        return static_cast<char>('0' + token - 5U);
    }
    if (token >= 15U && token <= 40U) {
        return static_cast<char>('a' + token - 15U);
    }
    return '\0';
}

auto PublicSuffixLookup::publicSuffixLabelCount(const text::String &host) const -> std::size_t {
    if (host.isEmpty()) {
        return 0U;
    }
    const auto characters = text::impl::UnsafeU8StringAccess{host}.dataSpan();
    if (characters.empty() || characters.back() == '.') {
        return 0U;
    }
    if (_data.maximumRuleLength >= cRuleBufferSize) {
        return 0U;
    }
    auto key = std::array<char, cRuleBufferSize>{};
    auto keyLength = std::size_t{};
    auto end = characters.size();
    auto matchedLabels = std::size_t{1U};
    for (auto labelCount = std::size_t{1U}; labelCount <= _data.maximumRuleLabelCount; ++labelCount) {
        auto start = end;
        while (start > 0U && characters[start - 1U] != '.') {
            --start;
        }
        const auto labelLength = end - start;
        const auto separatorLength = keyLength == 0U ? 0U : 1U;
        if (keyLength + separatorLength + labelLength > _data.maximumRuleLength) {
            break;
        }
        if (separatorLength != 0U) {
            key[keyLength] = '.';
            ++keyLength;
        }
        for (auto index = start; index < end; ++index) {
            key[keyLength] = characters[index];
            ++keyLength;
        }
        const auto ruleType = findRule(std::span<const char>{key.data(), keyLength});
        if (ruleType == RuleType::Exception) {
            return labelCount > 1U ? labelCount - 1U : 1U;
        }
        if (ruleType == RuleType::Exact) {
            matchedLabels = std::max(matchedLabels, labelCount);
        }
        if (ruleType == RuleType::Wildcard && start > 0U) {
            matchedLabels = std::max(matchedLabels, labelCount + 1U);
        }
        if (start == 0U) {
            break;
        }
        end = start - 1U;
    }
    return matchedLabels;
}

}
