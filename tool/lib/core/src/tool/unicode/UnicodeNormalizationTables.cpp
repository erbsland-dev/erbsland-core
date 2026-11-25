// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeNormalizationTables.hpp"

#include "UcdReader.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/IntegerParseOptions.hpp>
#include <erbsland/StringSplitter.hpp>

#include <algorithm>

namespace tool::unicode {

using namespace el::text::literals;

UnicodeNormalizationTables::UnicodeNormalizationTables(const UnicodeSource &source) {
    for (const auto &record : source.unicodeData().records()) {
        const auto codePoint = static_cast<uint32_t>(record.first);
        const auto combining = record.fields.get(el::ItemIndex{3}).toIntegerOrThrow<uint32_t>();
        if (combining != 0U) {
            _combining.emplace(codePoint, static_cast<uint8_t>(combining));
        }
        auto mappingText = record.fields.get(el::ItemIndex{5});
        if (mappingText.isEmpty()) {
            continue;
        }
        auto parts = el::StringList{};
        auto splitter = el::StringSplitter{mappingText, U' '};
        while (!splitter.isAtEnd()) {
            parts.append(splitter.next());
        }
        auto compatibility = false;
        auto index = el::ItemIndex{0};
        if (parts.get(index).startsWith("<"_el)) {
            if (!parts.get(index).endsWith(">"_el)) {
                throw el::ApplicationError{"Invalid Unicode Decomposition"_el, "Malformed compatibility tag."_el};
            }
            compatibility = true;
            index = el::ItemIndex{1};
        }
        auto mapping = std::vector<uint32_t>{};
        for (auto partIndex = index; partIndex.isWithin(parts.count()); ++partIndex) {
            mapping.push_back(parseHex(parts.get(partIndex)));
        }
        if (mapping.empty() || mapping.size() > 32U) {
            throw el::ApplicationError{
                "Invalid Unicode Decomposition"_el, "A decomposition mapping has an invalid length."_el};
        }
        _decompositions.emplace(codePoint, Decomposition{codePoint, std::move(mapping), compatibility});
    }
    const auto &dataset = source.dataset();
    auto exclusions = std::set<uint32_t>{};
    for (
        const auto &record :
        UcdReader{
            dataset.text("CompositionExclusions.txt"_el), dataset.sourcePath("CompositionExclusions.txt"_el).toString()}
            .parse(el::ItemCount{1})) {
        for (auto value = static_cast<uint32_t>(record.first); value <= static_cast<uint32_t>(record.last); ++value) {
            exclusions.insert(value);
        }
    }
    auto seenPairs = std::set<std::pair<uint32_t, uint32_t>>{};
    for (const auto &[codePoint, decomposition] : _decompositions) {
        if (decomposition.compatibility || decomposition.mapping.size() != 2U || exclusions.contains(codePoint)) {
            continue;
        }
        const auto first = decomposition.mapping[0];
        const auto second = decomposition.mapping[1];
        if (combiningClass(codePoint) != 0U || combiningClass(first) != 0U) {
            continue;
        }
        if (!seenPairs.insert({first, second}).second) {
            throw el::ApplicationError{
                "Invalid Unicode Composition"_el, "Two scalars define the same canonical composition pair."_el};
        }
        _compositions[first].emplace_back(second, codePoint);
    }
    for (auto &[starter, pairs] : _compositions) {
        std::sort(pairs.begin(), pairs.end());
    }
    validateRuntimeLimits();
    encode();
    const auto totalSize = _decompositionPages.size() * 4U + _decompositionData.size() + _combiningPages.size() * 4U +
        _combiningData.size() + _compositionPages.size() * 4U + _compositionData.size();
    if (_decompositionData.size() > 29000U || _combiningData.size() > 2100U || _compositionData.size() > 4700U ||
        totalSize > 36000U) {
        throw el::ApplicationError{
            "Unicode Normalization Data Exceeds Runtime Limit"_el,
            "The encoded normalization tables exceed their size budgets."_el};
    }
}

auto UnicodeNormalizationTables::parseHex(const el::String &text) -> uint32_t {
    auto options = el::IntegerParseOptions{}
                       .setFixedBase(el::IntegerBase::Hexadecimal)
                       .setMinimumDigits(el::CpCount{1})
                       .setMaximumDigits(el::CpCount{6})
                       .setFlags(el::IntegerParseFlags{});
    const auto value = text.toIntegerOrThrow<uint32_t>(options);
    if (value > 0x10FFFFU) {
        throw el::ApplicationError{"Invalid UCD Code Point"_el, "A code point exceeds U+10FFFF."_el};
    }
    return value;
}

auto UnicodeNormalizationTables::combiningClass(const uint32_t codePoint) const -> uint8_t {
    if (const auto it = _combining.find(codePoint); it != _combining.end()) {
        return it->second;
    }
    return 0U;
}

auto UnicodeNormalizationTables::expanded(
    const uint32_t codePoint, const bool compatibility, std::set<uint32_t> &active) const -> std::vector<uint32_t> {
    const auto it = _decompositions.find(codePoint);
    if (it == _decompositions.end() || (it->second.compatibility && !compatibility)) {
        return {codePoint};
    }
    if (!active.insert(codePoint).second) {
        throw el::ApplicationError{"Invalid Unicode Decomposition"_el, "Cyclic decomposition mapping."_el};
    }
    auto result = std::vector<uint32_t>{};
    for (const auto mapped : it->second.mapping) {
        auto part = expanded(mapped, compatibility, active);
        result.insert(result.end(), part.begin(), part.end());
        if (result.size() > 18U) {
            throw el::ApplicationError{
                "Unicode Normalization Data Exceeds Runtime Limit"_el,
                "Recursive decomposition exceeds the runtime buffer."_el};
        }
    }
    active.erase(codePoint);
    return result;
}

auto UnicodeNormalizationTables::depth(
    const uint32_t codePoint, const bool compatibility, std::set<uint32_t> &active) const -> uint32_t {
    const auto it = _decompositions.find(codePoint);
    if (it == _decompositions.end() || (it->second.compatibility && !compatibility)) {
        return 0U;
    }
    if (!active.insert(codePoint).second) {
        throw el::ApplicationError{"Invalid Unicode Decomposition"_el, "Cyclic decomposition mapping."_el};
    }
    auto result = 1U;
    for (const auto mapped : it->second.mapping) {
        result = std::max(result, 1U + depth(mapped, compatibility, active));
    }
    active.erase(codePoint);
    return result;
}

auto UnicodeNormalizationTables::compositionDepth(const uint32_t starter, std::set<uint32_t> &active) const
    -> uint32_t {
    if (!active.insert(starter).second) {
        throw el::ApplicationError{"Invalid Unicode Composition"_el, "Cyclic class-zero composition chain."_el};
    }
    auto result = 1U;
    if (const auto it = _compositions.find(starter); it != _compositions.end()) {
        for (const auto &[trailing, composite] : it->second) {
            if (combiningClass(trailing) == 0U) {
                result = std::max(result, 1U + compositionDepth(composite, active));
            }
        }
    }
    active.erase(starter);
    return result;
}

void UnicodeNormalizationTables::validateRuntimeLimits() const {
    auto maximumLength = 3U;
    auto maximumDepth = 1U;
    for (const auto compatibility : {false, true}) {
        for (const auto &[codePoint, decomposition] : _decompositions) {
            auto active = std::set<uint32_t>{};
            const auto sequence = expanded(codePoint, compatibility, active);
            maximumLength = std::max(maximumLength, static_cast<uint32_t>(sequence.size()));
            maximumDepth = std::max(maximumDepth, depth(codePoint, compatibility, active));
            if (combiningClass(sequence.front()) != 0U) {
                for (const auto scalar : sequence) {
                    if (combiningClass(scalar) == 0U) {
                        throw el::ApplicationError{
                            "Invalid Unicode Decomposition"_el,
                            "A nonstarter decomposition later introduces a starter."_el};
                    }
                }
            }
        }
    }
    auto maximumCompositionChain = 3U;
    for (const auto &[starter, pairs] : _compositions) {
        auto active = std::set<uint32_t>{};
        maximumCompositionChain = std::max(maximumCompositionChain, compositionDepth(starter, active));
    }
    if (maximumLength > 18U || maximumDepth > 3U || std::max(maximumLength, maximumCompositionChain) + 30U > 64U) {
        throw el::ApplicationError{
            "Unicode Normalization Data Exceeds Runtime Limit"_el,
            "The Unicode snapshot exceeds a normalization runtime bound."_el};
    }
}

}
