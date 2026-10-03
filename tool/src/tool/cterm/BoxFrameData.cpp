// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BoxFrameData.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>

#include <algorithm>
#include <limits>

namespace tool::cterm {

using namespace el::text::literals;

BoxFrameData::BoxFrameData(const std::vector<char32_t> &codePoints, const unicode::UnicodeData &unicodeData) {
    auto names = std::map<char32_t, el::String>{};
    for (const auto &record : unicodeData.records()) {
        if (record.first == record.last) {
            names.emplace(record.first, record.fields.get(el::ItemIndex{1}));
        }
    }
    for (const auto codePoint : codePoints) {
        if (_indexes.contains(codePoint) || !names.contains(codePoint)) {
            throw el::ApplicationError{
                "Invalid Box Frame Source"_el, "The selected repertoire contains a duplicate or unknown scalar."_el};
        }
        const auto attributes = attributesForName(names.at(codePoint));
        _indexes.emplace(codePoint, _characters.size());
        _names.set(names.at(codePoint), codePoint);
        _characters.push_back(BoxCharacter{codePoint, attributes});
        _exact[attributes] = codePoint;
    }
    if (_characters.empty() || _characters.size() > 255U) {
        throw el::ApplicationError{
            "Invalid Box Frame Source"_el, "The repertoire must contain between one and 255 characters."_el};
    }
}

auto BoxFrameData::character(const char32_t codePoint) const -> const BoxCharacter & {
    if (const auto it = _indexes.find(codePoint); it != _indexes.end()) {
        return _characters[it->second];
    }
    throw el::ApplicationError{"Invalid Box Frame Character"_el, "The selected character was not found."_el};
}

auto BoxFrameData::codePointForName(const el::String &name) const -> char32_t {
    if (const auto codePoint = _names.get(name)) {
        return *codePoint;
    }
    throw el::ApplicationError{"Invalid Box Frame Character"_el, "The selected UCD name was not found."_el};
}

auto BoxFrameData::packed(const BoxAttributes attributes) noexcept -> uint32_t {
    auto value = 0U;
    for (auto index = 0U; index < attributes.strokes.size(); ++index) {
        value |= static_cast<uint32_t>(attributes.strokes[index]) << (index * 4U);
    }
    return value;
}

auto BoxFrameData::best(const BoxAttributes ideal) const -> char32_t {
    if (const auto it = _exact.find(ideal); it != _exact.end()) {
        return it->second;
    }
    auto bestCharacter = _characters.front().codePoint;
    auto bestScore = std::numeric_limits<uint32_t>::max();
    for (const auto &entry : _characters) {
        const auto candidateScore = score(ideal, entry.attributes);
        if (candidateScore < bestScore) {
            bestScore = candidateScore;
            bestCharacter = entry.codePoint;
        }
    }
    return bestCharacter;
}

auto BoxFrameData::combine(const char32_t current, const char32_t overlay) const -> char32_t {
    const auto currentAttributes = character(current).attributes;
    const auto overlayAttributes = character(overlay).attributes;
    if (isCenterOnly(currentAttributes) && hasLines(overlayAttributes)) {
        return overlay;
    }
    if (hasLines(currentAttributes) && isCenterOnly(overlayAttributes)) {
        return current;
    }
    const auto ideal = merged(currentAttributes, overlayAttributes);
    if (const auto it = _exact.find(ideal); it != _exact.end()) {
        return it->second;
    }
    const auto candidate = best(ideal);
    if (score(ideal, character(candidate).attributes) + 100U < score(ideal, overlayAttributes)) {
        return candidate;
    }
    return overlay;
}

auto BoxFrameData::segments(const el::String &name) -> el::StringList {
    auto result = el::StringList{};
    auto remaining = name;
    while (const auto parts = remaining.splitAtFirst(" AND "_el)) {
        result.append(parts->first);
        remaining = parts->second;
    }
    result.append(remaining);
    return result;
}

void BoxFrameData::applyGroup(
    BoxAttributes &attributes, const el::String &group, const uint8_t stroke, const bool includeCenter) {
    auto positions = std::vector<std::size_t>{};
    if (group == "RIGHT"_el) {
        positions = {0};
    } else if (group == "DOWN"_el) {
        positions = {1};
    } else if (group == "LEFT"_el) {
        positions = {2};
    } else if (group == "UP"_el) {
        positions = {3};
    } else if (group == "VERTICAL"_el) {
        positions = {3, 1};
    } else if (group == "HORIZONTAL"_el) {
        positions = {2, 0};
    } else if (group == "LEFT DOWN"_el) {
        positions = {2, 1};
    } else if (group == "RIGHT DOWN"_el) {
        positions = {0, 1};
    } else if (group == "LEFT UP"_el) {
        positions = {2, 3};
    } else if (group == "RIGHT UP"_el) {
        positions = {0, 3};
    } else if (group == "DOWN HORIZONTAL"_el) {
        positions = {1, 2, 0};
    } else if (group == "UP HORIZONTAL"_el) {
        positions = {3, 2, 0};
    } else if (group == "RIGHT VERTICAL"_el) {
        positions = {0, 3, 1};
    } else if (group == "LEFT VERTICAL"_el) {
        positions = {2, 3, 1};
    } else {
        throw el::ApplicationError{
            "Invalid Box Drawing Name"_el, el::StringFormat{"Unsupported position group: {}"_el}.build(group)};
    }
    for (const auto index : positions) {
        attributes.strokes[index] = stroke;
    }
    if (includeCenter) {
        attributes.strokes[6] = std::max(attributes.strokes[6], stroke);
    }
}

auto BoxFrameData::styleAndGroup(const el::String &segment, const uint8_t inherited) -> std::pair<uint8_t, el::String> {
    auto normalized = segment.replacedAll(" SINGLE"_el, " LIGHT"_el);
    auto dash = 0U;
    if (normalized.contains(" QUADRUPLE DASH"_el)) {
        normalized = normalized.replacedAll(" QUADRUPLE DASH"_el, ""_el);
        dash = 1U;
    } else if (normalized.contains(" TRIPLE DASH"_el)) {
        normalized = normalized.replacedAll(" TRIPLE DASH"_el, ""_el);
        dash = 2U;
    } else if (normalized.contains(" DOUBLE DASH"_el)) {
        normalized = normalized.replacedAll(" DOUBLE DASH"_el, ""_el);
        dash = 3U;
    }
    auto stroke = inherited;
    auto group = normalized;
    for (
        const auto &[label, value] : std::array<std::pair<el::String, uint8_t>, 3>{
            std::pair{"LIGHT"_el, uint8_t{7}}, {"HEAVY"_el, uint8_t{9}}, {"DOUBLE"_el, uint8_t{8}}}) {
        if (normalized.startsWith(el::join(label, " "_el))) {
            stroke = value;
            group = normalized.replacedAll(el::join(label, " "_el), ""_el);
            break;
        }
        if (normalized.endsWith(el::join(" "_el, label))) {
            stroke = value;
            group = normalized.replacedAll(el::join(" "_el, label), ""_el);
            break;
        }
    }
    if (stroke == 0U) {
        throw el::ApplicationError{"Invalid Box Drawing Name"_el, "A stroke style cannot be inferred."_el};
    }
    if (dash == 1U) {
        stroke = stroke == 9U ? 2U : 1U;
    } else if (dash == 2U) {
        stroke = stroke == 9U ? 4U : 3U;
    } else if (dash == 3U) {
        stroke = stroke == 9U ? 6U : 5U;
    }
    return {stroke, group};
}

auto BoxFrameData::attributesForName(const el::String &name) -> BoxAttributes {
    auto result = BoxAttributes{};
    if (name == "SPACE"_el) {
        return result;
    }
    if (name == "BULLET OPERATOR"_el || name == "BLACK SMALL SQUARE"_el || name == "WHITE SMALL SQUARE"_el) {
        if (name == "BULLET OPERATOR"_el) {
            result.strokes[6] = 7U;
        } else if (name == "BLACK SMALL SQUARE"_el) {
            result.strokes[6] = 9U;
        } else {
            result.strokes[6] = 8U;
        }
        return result;
    }
    if (!name.startsWith("BOX DRAWINGS "_el)) {
        throw el::ApplicationError{
            "Invalid Box Frame Source"_el, "A selected scalar is not a Unicode box-drawing character."_el};
    }
    const auto boxName = name.replacedAll("BOX DRAWINGS "_el, ""_el);
    if (boxName.startsWith("LIGHT ARC "_el)) {
        for (const auto &part : segments(boxName.replacedAll("LIGHT ARC "_el, ""_el))) {
            applyGroup(result, part, 7U, false);
        }
        return result;
    }
    if (boxName == "LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT"_el) {
        result.strokes[4] = 7U;
        result.strokes[6] = 7U;
        return result;
    }
    if (boxName == "LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT"_el) {
        result.strokes[5] = 7U;
        result.strokes[6] = 7U;
        return result;
    }
    if (boxName == "LIGHT DIAGONAL CROSS"_el) {
        result.strokes[4] = 7U;
        result.strokes[5] = 7U;
        result.strokes[6] = 7U;
        return result;
    }
    auto inherited = uint8_t{};
    for (const auto &part : segments(boxName)) {
        const auto [stroke, group] = styleAndGroup(part, inherited);
        applyGroup(result, group, stroke, true);
        inherited = stroke;
    }
    return result;
}

auto BoxFrameData::hasLines(const BoxAttributes &attributes) noexcept -> bool {
    for (auto index = 0U; index < 6U; ++index) {
        if (attributes.strokes[index] != 0U) {
            return true;
        }
    }
    return false;
}

auto BoxFrameData::isCenterOnly(const BoxAttributes &attributes) noexcept -> bool {
    return attributes.strokes[6] != 0U && !hasLines(attributes);
}

auto BoxFrameData::score(const BoxAttributes ideal, const BoxAttributes candidate) noexcept -> uint32_t {
    constexpr auto cWeights = std::array<uint32_t, 7>{4U, 4U, 4U, 4U, 5U, 5U, 6U};
    auto missing = 0U;
    auto distance = 0U;
    auto extra = 0U;
    for (auto index = 0U; index < 7U; ++index) {
        const auto expected = ideal.strokes[index];
        const auto actual = candidate.strokes[index];
        if (expected != 0U && actual == 0U) {
            missing += cWeights[index];
        } else if (expected == 0U && actual != 0U) {
            extra += cWeights[index] * (2U + actual);
        } else if (expected != 0U && actual != 0U) {
            distance += cWeights[index] * static_cast<uint32_t>(std::abs(int(expected) - int(actual)));
        }
    }
    return missing * 10000U + distance * 100U + extra;
}

auto BoxFrameData::merged(const BoxAttributes left, const BoxAttributes right) noexcept -> BoxAttributes {
    auto result = BoxAttributes{};
    for (auto index = 0U; index < 7U; ++index) {
        result.strokes[index] = std::max(left.strokes[index], right.strokes[index]);
    }
    return result;
}

}
