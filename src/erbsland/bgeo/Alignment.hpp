// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AlignmentFlags.hpp"
#include "BlockCoordinate.hpp"

#include "../util/HashHelper.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace erbsland::bgeo {

/// Alignment of text or graphics.
/// Compared to the low-level `AlignmentFlags`, this object is bound to valid states.
/// Horizontal and vertical alignment flags are mutually exclusive but optional.
/// Also, this class is immutable and designed to be used as a function parameter.
/// @seedoc{/reference/bgeo/alignment_and_orientation}
/// @tested{AlignmentTest}
class Alignment final {
    struct PrivateTag {};

public:
    static const Alignment Left;         ///< Aligned to the left edge of the box.
    static const Alignment HCenter;      ///< Aligned to the horizontal center of the box.
    static const Alignment Right;        ///< Aligned to the right edge of the box.
    static const Alignment Top;          ///< Aligned to the top edge of the box.
    static const Alignment VCenter;      ///< Aligned to the vertical center of the box.
    static const Alignment Bottom;       ///< Aligned to the bottom edge of the box.
    static const Alignment TopLeft;      ///< Aligned to the top-left corner of the box.
    static const Alignment TopCenter;    ///< Aligned to the top-center of the box.
    static const Alignment TopRight;     ///< Aligned to the top-right corner of the box.
    static const Alignment CenterLeft;   ///< Aligned to the center-left of the box.
    static const Alignment Center;       ///< Aligned to the center of the box.
    static const Alignment CenterRight;  ///< Aligned to the center-right of the box.
    static const Alignment BottomLeft;   ///< Aligned to the bottom-left corner of the box.
    static const Alignment BottomCenter; ///< Aligned to the bottom-center of the box.
    static const Alignment BottomRight;  ///< Aligned to the bottom-right corner of the box.

public:
    /// Create a top-left alignment.
    constexpr Alignment() noexcept = default;

    /// Create an alignment from the low-level alignment flags.
    /// Ensures that only one horizontal and one vertical flag are set.
    constexpr Alignment(const AlignmentFlags value) noexcept : // NOLINT(*-explicit-constructor)
        _value{makeExclusive(value)} {}

    // defaults
    ~Alignment() = default;
    Alignment(const Alignment &) = default;
    auto operator=(const Alignment &) -> Alignment & = default;

public: // operators
    /// Test if two alignments are equal.
    auto operator==(const Alignment &) const noexcept -> bool = default;
    /// Test if two alignments are different.
    auto operator!=(const Alignment &) const noexcept -> bool = default;

public: // tests
    /// Test if this alignment is on the left side.
    [[nodiscard]] constexpr auto isLeft() const noexcept -> bool {
        return horizontal().toRawValue() == AlignmentFlag::Left;
    }
    /// Test if this alignment is horizontally centered.
    [[nodiscard]] constexpr auto isHorizontalCenter() const noexcept -> bool {
        return horizontal().toRawValue() == AlignmentFlag::HCenter;
    }
    /// Test if this alignment is on the right side.
    [[nodiscard]] constexpr auto isRight() const noexcept -> bool {
        return horizontal().toRawValue() == AlignmentFlag::Right;
    }
    /// Test if this alignment is at the top.
    [[nodiscard]] constexpr auto isTop() const noexcept -> bool {
        return vertical().toRawValue() == AlignmentFlag::Top;
    }
    /// Test if this alignment is vertically centered.
    [[nodiscard]] constexpr auto isVerticalCenter() const noexcept -> bool {
        return vertical().toRawValue() == AlignmentFlag::VCenter;
    }
    /// Test if this alignment is at the bottom.
    [[nodiscard]] constexpr auto isBottom() const noexcept -> bool {
        return vertical().toRawValue() == AlignmentFlag::Bottom;
    }

public: // filter
    /// Extract the horizontal alignment component.
    [[nodiscard]] constexpr auto horizontal() const noexcept -> Alignment {
        return Alignment{_value & AlignmentFlag::HorizontalMask, PrivateTag{}};
    }
    /// Extract the vertical alignment component.
    [[nodiscard]] constexpr auto vertical() const noexcept -> Alignment {
        return Alignment{_value & AlignmentFlag::VerticalMask, PrivateTag{}};
    }

public: // layout
    /// Calculate the horizontal offset for content aligned in an available width.
    [[nodiscard]] auto horizontalOffset(
        const BlockCoordinate availableWidth, const BlockCoordinate contentWidth) const noexcept -> BlockCoordinate {
        const auto freeSpace = availableWidth - contentWidth;
        if (isHorizontalCenter()) {
            return freeSpace / 2;
        }
        if (isRight()) {
            return freeSpace;
        }
        return BlockCoordinate{0};
    }
    /// @overload
    [[nodiscard]] auto horizontalOffset(const int availableWidth, const int contentWidth) const noexcept
        -> BlockCoordinate {
        return horizontalOffset(BlockCoordinate{availableWidth}, BlockCoordinate{contentWidth});
    }
    /// Calculate the vertical offset for content aligned in an available height.
    [[nodiscard]] auto verticalOffset(
        const BlockCoordinate availableHeight, const BlockCoordinate contentHeight) const noexcept -> BlockCoordinate {
        const auto freeSpace = availableHeight - contentHeight;
        if (isVerticalCenter()) {
            return freeSpace / 2;
        }
        if (isBottom()) {
            return freeSpace;
        }
        return BlockCoordinate{0};
    }
    /// @overload
    [[nodiscard]] auto verticalOffset(const int availableHeight, const int contentHeight) const noexcept
        -> BlockCoordinate {
        return verticalOffset(BlockCoordinate{availableHeight}, BlockCoordinate{contentHeight});
    }

public: // conversion
    /// Get a hash for this alignment.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return util::createHash(_value); }
    /// Get the underlying alignment flags.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> AlignmentFlags { return _value; }

private:
    /// Unchecked private constructor.
    explicit constexpr Alignment(const AlignmentFlags value, PrivateTag) : _value{value} {}

    /// Make sure only one alignment flag per orientation is set.
    /// If multiple flags are set, only keep the first one.
    [[nodiscard]] constexpr static auto makeExclusive(AlignmentFlags rawFlags) noexcept -> AlignmentFlags {
        AlignmentFlags result;
        if (rawFlags.isSet(AlignmentFlag::Left)) {
            result |= AlignmentFlag::Left;
        } else if (rawFlags.isSet(AlignmentFlag::HCenter)) {
            result |= AlignmentFlag::HCenter;
        } else if (rawFlags.isSet(AlignmentFlag::Right)) {
            result |= AlignmentFlag::Right;
        }
        if (rawFlags.isSet(AlignmentFlag::Top)) {
            result |= AlignmentFlag::Top;
        } else if (rawFlags.isSet(AlignmentFlag::VCenter)) {
            result |= AlignmentFlag::VCenter;
        } else if (rawFlags.isSet(AlignmentFlag::Bottom)) {
            result |= AlignmentFlag::Bottom;
        }
        return result;
    }

private:
    AlignmentFlags _value{AlignmentFlag::TopLeft}; ///< The internal enum flags.
};

inline constexpr Alignment Alignment::Left{AlignmentFlag::Left};
inline constexpr Alignment Alignment::HCenter{AlignmentFlag::HCenter};
inline constexpr Alignment Alignment::Right{AlignmentFlag::Right};
inline constexpr Alignment Alignment::Top{AlignmentFlag::Top};
inline constexpr Alignment Alignment::VCenter{AlignmentFlag::VCenter};
inline constexpr Alignment Alignment::Bottom{AlignmentFlag::Bottom};
inline constexpr Alignment Alignment::TopLeft{AlignmentFlag::TopLeft};
inline constexpr Alignment Alignment::TopCenter{AlignmentFlag::TopCenter};
inline constexpr Alignment Alignment::TopRight{AlignmentFlag::TopRight};
inline constexpr Alignment Alignment::CenterLeft{AlignmentFlag::CenterLeft};
inline constexpr Alignment Alignment::Center{AlignmentFlag::Center};
inline constexpr Alignment Alignment::CenterRight{AlignmentFlag::CenterRight};
inline constexpr Alignment Alignment::BottomLeft{AlignmentFlag::BottomLeft};
inline constexpr Alignment Alignment::BottomCenter{AlignmentFlag::BottomCenter};
inline constexpr Alignment Alignment::BottomRight{AlignmentFlag::BottomRight};

}

template <>
struct std::hash<erbsland::bgeo::Alignment> {
    auto operator()(const erbsland::bgeo::Alignment &alignment) const noexcept -> std::size_t {
        return alignment.hash();
    }
};
