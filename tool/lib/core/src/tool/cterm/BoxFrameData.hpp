// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BoxAttributes.hpp"
#include "BoxCharacter.hpp"

#include "../unicode/UnicodeData.hpp"

#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringMap.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <vector>

namespace tool::cterm {

/// Parse the selected repertoire and score terminal frame combinations.
/// @tested{BoxFrameToolTest}
class BoxFrameData final {
public:
    /// Build selected character attributes from ordered code points and UCD names.
    /// @param codePoints The checked-in selected repertoire.
    /// @param unicodeData Verified UCD records supplying character names.
    BoxFrameData(const std::vector<char32_t> &codePoints, const unicode::UnicodeData &unicodeData);

public: // accessors
    /// Get selected characters in the source-file order.
    [[nodiscard]] auto characters() const noexcept -> const std::vector<BoxCharacter> & { return _characters; }
    /// Find a selected character by its code point.
    [[nodiscard]] auto character(char32_t codePoint) const -> const BoxCharacter &;
    /// Find one selected character by its verified UCD name.
    [[nodiscard]] auto codePointForName(const el::String &name) const -> char32_t;
    /// Pack one attribute set into the runtime word format.
    [[nodiscard]] static auto packed(BoxAttributes attributes) noexcept -> uint32_t;
    /// Find the best supported character for a desired attribute set.
    [[nodiscard]] auto best(BoxAttributes ideal) const -> char32_t;
    /// Combine a current character and overlay according to the runtime policy.
    [[nodiscard]] auto combine(char32_t current, char32_t overlay) const -> char32_t;

private:
    /// Interpret one source-backed UCD name.
    [[nodiscard]] static auto attributesForName(const el::String &name) -> BoxAttributes;
    /// Apply the positions named by one UCD name segment.
    static void applyGroup(BoxAttributes &attributes, const el::String &group, uint8_t stroke, bool includeCenter);
    /// Read one style and group from a UCD name segment.
    [[nodiscard]] static auto styleAndGroup(const el::String &segment, uint8_t inherited)
        -> std::pair<uint8_t, el::String>;
    /// Split a UCD box name at each AND separator.
    [[nodiscard]] static auto segments(const el::String &name) -> el::StringList;
    /// Determine if an attribute set has a line segment.
    [[nodiscard]] static auto hasLines(const BoxAttributes &attributes) noexcept -> bool;
    /// Determine if only the center marker is active.
    [[nodiscard]] static auto isCenterOnly(const BoxAttributes &attributes) noexcept -> bool;
    /// Score a selected character against ideal strokes.
    [[nodiscard]] static auto score(BoxAttributes ideal, BoxAttributes candidate) noexcept -> uint32_t;
    /// Merge two attribute sets by keeping the stronger stroke at every position.
    [[nodiscard]] static auto merged(BoxAttributes left, BoxAttributes right) noexcept -> BoxAttributes;

private:
    std::vector<BoxCharacter> _characters;    ///< Source-ordered repertoire.
    std::map<char32_t, std::size_t> _indexes; ///< Code-point lookup.
    el::StringMap<char32_t> _names;           ///< Verified UCD name lookup.
    std::map<BoxAttributes, char32_t> _exact; ///< Last source character for identical attributes.
};

}
