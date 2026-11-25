// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "UnicodeNormalizationTables.hpp"

#include <erbsland/ApplicationError.hpp>

namespace tool::unicode {

using namespace el::text::literals;

void UnicodeNormalizationTables::encodeUnsigned(std::vector<uint8_t> &output, uint32_t value) {
    do {
        auto byte = static_cast<uint8_t>(value & 0x7FU);
        value >>= 7U;
        if (value != 0U) {
            byte |= 0x80U;
        }
        output.push_back(byte);
    } while (value != 0U);
}

auto UnicodeNormalizationTables::packPage(const uint32_t page, const std::size_t offset) -> uint32_t {
    if (page > 0x1FFU || offset > 0x7FFFFFU) {
        throw el::ApplicationError{
            "Unicode Normalization Data Exceeds Runtime Limit"_el,
            "A normalization page or byte offset cannot be encoded."_el};
    }
    return (static_cast<uint32_t>(offset) << 9U) | page;
}

void UnicodeNormalizationTables::encode() {
    auto previousPage = 0xFFFFFFFFU;
    auto previousCodePoint = 0U;
    for (const auto &[codePoint, entry] : _decompositions) {
        const auto page = codePoint >> 12U;
        if (page != previousPage) {
            _decompositionPages.push_back(packPage(page, _decompositionData.size()));
            previousPage = page;
            previousCodePoint = page << 12U;
        }
        encodeUnsigned(_decompositionData, codePoint - previousCodePoint);
        auto header = static_cast<uint8_t>(entry.mapping.size() - 1U);
        if (entry.compatibility) {
            header |= 0x20U;
        }
        _decompositionData.push_back(header);
        for (const auto mapped : entry.mapping) {
            encodeUnsigned(_decompositionData, mapped);
        }
        previousCodePoint = codePoint;
    }
    previousPage = 0xFFFFFFFFU;
    previousCodePoint = 0U;
    for (const auto &[codePoint, combiningClass] : _combining) {
        const auto page = codePoint >> 12U;
        if (page != previousPage) {
            _combiningPages.push_back(packPage(page, _combiningData.size()));
            previousPage = page;
            previousCodePoint = page << 12U;
        }
        encodeUnsigned(_combiningData, codePoint - previousCodePoint);
        _combiningData.push_back(combiningClass);
        previousCodePoint = codePoint;
    }
    previousPage = 0xFFFFFFFFU;
    previousCodePoint = 0U;
    for (const auto &[starter, pairs] : _compositions) {
        const auto page = starter >> 12U;
        if (page != previousPage) {
            _compositionPages.push_back(packPage(page, _compositionData.size()));
            previousPage = page;
            previousCodePoint = page << 12U;
        }
        encodeUnsigned(_compositionData, starter - previousCodePoint);
        encodeUnsigned(_compositionData, static_cast<uint32_t>(pairs.size()));
        auto previousTrailing = 0U;
        for (const auto &[trailing, composite] : pairs) {
            encodeUnsigned(_compositionData, trailing - previousTrailing);
            encodeUnsigned(_compositionData, composite);
            previousTrailing = trailing;
        }
        previousCodePoint = starter;
    }
}

}
