// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EngineDebug.hpp"
#include "EngineThread.hpp"

#include "../text/Category.hpp"
#include "../text/Character.hpp"

#include "../../../text/StringFormat.hpp"
#include "../../InputPosition.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::re::impl {

/// The state related to one character in the stream.
class EngineCharacterState final {
public:
    EngineCharacterState() = default;
    ~EngineCharacterState() = default;

    // prevent copy and assignments.
    EngineCharacterState(const EngineCharacterState &) = delete;
    EngineCharacterState &operator=(const EngineCharacterState &) = delete;
    EngineCharacterState(EngineCharacterState &&) noexcept = delete;
    EngineCharacterState &operator=(EngineCharacterState &&) noexcept = delete;

public:
    void swap(EngineCharacterState &other) noexcept {
        std::swap(position, other.position);
        std::swap(character, other.character);
        std::swap(caseFoldedCharacter, other.caseFoldedCharacter);
        std::swap(categoryMask, other.categoryMask);
        std::swap(hasCaseInsensitiveOperations, other.hasCaseInsensitiveOperations);
        threads.swap(other.threads);
    }

    void clear() noexcept {
        position = 0;
        character = text::Char::noCodePoint();
        caseFoldedCharacter = text::Char::noCodePoint();
        categoryMask = Category::cNotComputed;
        hasCaseInsensitiveOperations = false;
        threads.clear();
    }

    void resetForNextFind() noexcept { threads.clear(); }

#ifdef ERBSLAND_RE_ENGINE_DEBUG_ENABLED
    [[nodiscard]] auto toDebugString() -> text::String {
        return text::StringFormat{"SubState(position={},character={},caseFoldedCharacter={},categoryMask=0x{:08X},"
                                  "hasCaseInsensitiveOperations={},threadCount={})"}
            .build(
                position,
                character,
                caseFoldedCharacter,
                static_cast<std::uint32_t>(categoryMask),
                hasCaseInsensitiveOperations,
                threads.size());
    }
#endif

public:                                                        // public fields
    InputPosition position{0};                                 ///< The start position of the character.
    text::Char character{text::Char::noCodePoint()};           ///< The character.
    text::Char caseFoldedCharacter{text::Char::noCodePoint()}; ///< A case-folded version of the character.
    /// The category mask of the character.
    /// Zero means that no category mask was computed yet. This works because every character is at least
    /// in one Unicode category, and therefore, at least one bit must be set if the mask was calculated.
    Category::Mask categoryMask{Category::cNotComputed};
    EngineThreadList threads;                 ///< A list of threads.
    bool hasCaseInsensitiveOperations{false}; ///< If the threads contain at least one case-insensitive operation.
};

}
