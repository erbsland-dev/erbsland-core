// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Identifier.hpp"

#include <functional>
#include <limits>
#include <stdexcept>

namespace erbsland::cterm::theme {

/// A fast theme part identifier.
///
/// Parts identify the visual role inside an element, for example a scrollbar track or thumb.
class Part final {
public:
    using Id = Identifier<IdentifierType::Part>; ///< The underlying part identifier.

public:
    /// Create an empty part identifier.
    constexpr Part() noexcept = default;
    /// Create a part from a part identifier.
    /// @param id The non-zero part identifier.
    constexpr Part(const Id id) noexcept : _id{id} {}

public: // operators
    /// Compare two part identifiers.
    auto operator==(const Part &other) const noexcept -> bool = default;
    /// Compare two part identifiers.
    auto operator!=(const Part &other) const noexcept -> bool = default;

public:
    /// Get the numeric identifier.
    /// @return The numeric identifier.
    [[nodiscard]] constexpr auto value() const noexcept -> uint16_t { return _id.value(); }
    /// Test if this identifier is non-empty.
    /// @return `true` if this part has a non-zero identifier.
    [[nodiscard]] constexpr auto isValid() const noexcept -> bool { return _id.isValid(); }
    /// Create a custom part identifier.
    /// @param id A stable application-defined positive identifier.
    /// @return The custom part identifier.
    /// @throws std::invalid_argument If `id` is not positive.
    /// @throws std::out_of_range If `id` exceeds the supported custom identifier range.
    [[nodiscard]] static constexpr auto custom(const int32_t id) -> Part {
        if (id <= 0) {
            throw std::invalid_argument{"Custom theme part identifiers must be positive."};
        }
        if (id > std::numeric_limits<uint16_t>::max() - cCustomOffset) {
            throw std::out_of_range{"Custom theme part identifier is too large."};
        }
        return Part{Id{static_cast<uint16_t>(id + cCustomOffset)}};
    }
    /// Get a stable hash for this part identifier.
    /// @return The hash value.
    [[nodiscard]] constexpr auto hash() const noexcept -> std::size_t { return _id.hash(); }

public:
    /// No specific part.
    static const Part None;
    /// Background fill.
    static const Part Background;
    /// Text content.
    static const Part Text;
    /// Border or frame.
    static const Part Border;
    /// Scrollbar or slider track.
    static const Part Track;
    /// Scrollbar or slider thumb.
    static const Part Thumb;
    /// Decrease button or indicator.
    static const Part Decrease;
    /// Increase button or indicator.
    static const Part Increase;
    /// Generic indicator part.
    static const Part Indicator;
    /// Key label in the shortcut help.
    static const Part Key;
    /// Bracket text around a key label in shortcut help.
    static const Part KeyBracket;
    /// The action name after a keyboard shortcut.
    static const Part ActionName;
    /// Title text in a decorated element.
    static const Part Title;
    /// Brackets around the title text.
    static const Part TitleBracket;
    /// Spacing configuration between repeated elements.
    static const Part Spacing;
    /// Ellipsis marker for cropped text.
    static const Part Ellipsis;

private:
    static constexpr auto cCustomOffset = uint16_t{1000};

private:
    Id _id; ///< The numeric part identifier.
};

inline constexpr Part Part::None{Id{0}};
inline constexpr Part Part::Background{Id{1}};
inline constexpr Part Part::Text{Id{2}};
inline constexpr Part Part::Border{Id{3}};
inline constexpr Part Part::Track{Id{4}};
inline constexpr Part Part::Thumb{Id{5}};
inline constexpr Part Part::Decrease{Id{6}};
inline constexpr Part Part::Increase{Id{7}};
inline constexpr Part Part::Indicator{Id{8}};
inline constexpr Part Part::Key{Id{9}};
inline constexpr Part Part::KeyBracket{Id{10}};
inline constexpr Part Part::ActionName{Id{11}};
inline constexpr Part Part::Title{Id{12}};
inline constexpr Part Part::TitleBracket{Id{13}};
inline constexpr Part Part::Spacing{Id{14}};
inline constexpr Part Part::Ellipsis{Id{15}};

}

template <>
struct std::hash<erbsland::cterm::theme::Part> {
    auto operator()(const erbsland::cterm::theme::Part &part) const noexcept -> std::size_t { return part.hash(); }
};
