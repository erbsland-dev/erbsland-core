// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BlockAnchorFlag.hpp"

#include "../util/HashHelper.hpp"

#include <functional>

namespace erbsland::bgeo {

/// Anchor for positions inside a block rectangle or size.
/// Horizontal and vertical anchor flags are mutually exclusive but optional.
/// @seedoc{/reference/bgeo/block_geometry}
/// @tested{BlockAnchorTest}
class BlockAnchor final {
    struct PrivateTag {};

public:
    static const BlockAnchor Left;         ///< Anchored to the left edge.
    static const BlockAnchor HCenter;      ///< Anchored to the horizontal center.
    static const BlockAnchor Right;        ///< Anchored to the right edge.
    static const BlockAnchor Top;          ///< Anchored to the top edge.
    static const BlockAnchor VCenter;      ///< Anchored to the vertical center.
    static const BlockAnchor Bottom;       ///< Anchored to the bottom edge.
    static const BlockAnchor TopLeft;      ///< Anchored to the top-left corner.
    static const BlockAnchor TopCenter;    ///< Anchored to the top-center edge.
    static const BlockAnchor TopRight;     ///< Anchored to the top-right corner.
    static const BlockAnchor CenterLeft;   ///< Anchored to the center-left edge.
    static const BlockAnchor Center;       ///< Anchored to the center.
    static const BlockAnchor CenterRight;  ///< Anchored to the center-right edge.
    static const BlockAnchor BottomLeft;   ///< Anchored to the bottom-left corner.
    static const BlockAnchor BottomCenter; ///< Anchored to the bottom-center edge.
    static const BlockAnchor BottomRight;  ///< Anchored to the bottom-right corner.

public:
    /// Create a top-left anchor.
    constexpr BlockAnchor() noexcept = default;

    /// Create an anchor from low-level anchor flags.
    constexpr BlockAnchor(const BlockAnchorFlags value) noexcept : // NOLINT(*-explicit-constructor)
        _value{makeExclusive(value)} {}

    // defaults
    ~BlockAnchor() = default;
    BlockAnchor(const BlockAnchor &) = default;
    /// Assign an anchor.
    auto operator=(const BlockAnchor &) -> BlockAnchor & = default;

public: // operators
    auto operator==(const BlockAnchor &) const noexcept -> bool = default;
    /// Compare two anchors for inequality.
    auto operator!=(const BlockAnchor &) const noexcept -> bool = default;

    /// Combine two anchors and normalize conflicting components.
    [[nodiscard]] friend constexpr auto operator|(BlockAnchor lhs, BlockAnchor rhs) noexcept -> BlockAnchor {
        return BlockAnchor{lhs._value | rhs._value};
    }

public: // tests
    /// Test if this anchor uses the left horizontal edge.
    [[nodiscard]] constexpr auto isLeft() const noexcept -> bool {
        return horizontal().toRawValue() == BlockAnchorFlag::Left;
    }
    /// Test if this anchor uses the horizontal center.
    [[nodiscard]] constexpr auto isHorizontalCenter() const noexcept -> bool {
        return horizontal().toRawValue() == BlockAnchorFlag::HCenter;
    }
    /// Test if this anchor uses the right horizontal edge.
    [[nodiscard]] constexpr auto isRight() const noexcept -> bool {
        return horizontal().toRawValue() == BlockAnchorFlag::Right;
    }
    /// Test if this anchor uses the top vertical edge.
    [[nodiscard]] constexpr auto isTop() const noexcept -> bool {
        return vertical().toRawValue() == BlockAnchorFlag::Top;
    }
    /// Test if this anchor uses the vertical center.
    [[nodiscard]] constexpr auto isVerticalCenter() const noexcept -> bool {
        return vertical().toRawValue() == BlockAnchorFlag::VCenter;
    }
    /// Test if this anchor uses the bottom vertical edge.
    [[nodiscard]] constexpr auto isBottom() const noexcept -> bool {
        return vertical().toRawValue() == BlockAnchorFlag::Bottom;
    }

public: // filter
    /// Return only the horizontal anchor component.
    [[nodiscard]] constexpr auto horizontal() const noexcept -> BlockAnchor {
        return BlockAnchor{_value & BlockAnchorFlag::HorizontalMask, PrivateTag{}};
    }
    /// Return only the vertical anchor component.
    [[nodiscard]] constexpr auto vertical() const noexcept -> BlockAnchor {
        return BlockAnchor{_value & BlockAnchorFlag::VerticalMask, PrivateTag{}};
    }

public: // conversion
    /// Create a hash value for this anchor.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return util::createHash(_value); }
    /// Return the normalized raw anchor flags.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> BlockAnchorFlags { return _value; }

private:
    /// Create an anchor from normalized flags without further normalization.
    /// @param value The normalized anchor flags.
    explicit constexpr BlockAnchor(const BlockAnchorFlags value, PrivateTag) : _value{value} {}

    /// Normalize horizontal and vertical flags by retaining their first matching values.
    [[nodiscard]] constexpr static auto makeExclusive(BlockAnchorFlags rawFlags) noexcept -> BlockAnchorFlags {
        BlockAnchorFlags result;
        if (rawFlags.isSet(BlockAnchorFlag::Left)) {
            result |= BlockAnchorFlag::Left;
        } else if (rawFlags.isSet(BlockAnchorFlag::HCenter)) {
            result |= BlockAnchorFlag::HCenter;
        } else if (rawFlags.isSet(BlockAnchorFlag::Right)) {
            result |= BlockAnchorFlag::Right;
        }
        if (rawFlags.isSet(BlockAnchorFlag::Top)) {
            result |= BlockAnchorFlag::Top;
        } else if (rawFlags.isSet(BlockAnchorFlag::VCenter)) {
            result |= BlockAnchorFlag::VCenter;
        } else if (rawFlags.isSet(BlockAnchorFlag::Bottom)) {
            result |= BlockAnchorFlag::Bottom;
        }
        return result;
    }

private:
    BlockAnchorFlags _value{BlockAnchorFlag::TopLeft}; ///< The internal anchor flags.
};

inline constexpr BlockAnchor BlockAnchor::Left{BlockAnchorFlag::Left};
inline constexpr BlockAnchor BlockAnchor::HCenter{BlockAnchorFlag::HCenter};
inline constexpr BlockAnchor BlockAnchor::Right{BlockAnchorFlag::Right};
inline constexpr BlockAnchor BlockAnchor::Top{BlockAnchorFlag::Top};
inline constexpr BlockAnchor BlockAnchor::VCenter{BlockAnchorFlag::VCenter};
inline constexpr BlockAnchor BlockAnchor::Bottom{BlockAnchorFlag::Bottom};
inline constexpr BlockAnchor BlockAnchor::TopLeft{BlockAnchorFlag::TopLeft};
inline constexpr BlockAnchor BlockAnchor::TopCenter{BlockAnchorFlag::TopCenter};
inline constexpr BlockAnchor BlockAnchor::TopRight{BlockAnchorFlag::TopRight};
inline constexpr BlockAnchor BlockAnchor::CenterLeft{BlockAnchorFlag::CenterLeft};
inline constexpr BlockAnchor BlockAnchor::Center{BlockAnchorFlag::Center};
inline constexpr BlockAnchor BlockAnchor::CenterRight{BlockAnchorFlag::CenterRight};
inline constexpr BlockAnchor BlockAnchor::BottomLeft{BlockAnchorFlag::BottomLeft};
inline constexpr BlockAnchor BlockAnchor::BottomCenter{BlockAnchorFlag::BottomCenter};
inline constexpr BlockAnchor BlockAnchor::BottomRight{BlockAnchorFlag::BottomRight};

}

template <>
struct std::hash<erbsland::bgeo::BlockAnchor> {
    auto operator()(const erbsland::bgeo::BlockAnchor &anchor) const noexcept -> std::size_t { return anchor.hash(); }
};
