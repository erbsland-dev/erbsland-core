// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnchorFlags.hpp"

#include "../util/HashHelper.hpp"

#include <cstddef>
#include <functional>

namespace erbsland::geometry {

/// Selects a relative position in a two-dimensional area.
/// An anchor contains at most one horizontal and one vertical component. It is suitable for choosing a corner, edge,
/// or center when positioning geometry or resolving a value from a two-dimensional grid.
/// @seedoc{/reference/geometry/geometry}
/// @tested{AnchorTest}
class Anchor final {
    struct PrivateTag {};

public:
    static const Anchor Left;         ///< The left edge.
    static const Anchor HCenter;      ///< The horizontal center.
    static const Anchor Right;        ///< The right edge.
    static const Anchor Top;          ///< The top edge.
    static const Anchor VCenter;      ///< The vertical center.
    static const Anchor Bottom;       ///< The bottom edge.
    static const Anchor TopLeft;      ///< The top-left corner.
    static const Anchor TopCenter;    ///< The top-center edge.
    static const Anchor TopRight;     ///< The top-right corner.
    static const Anchor CenterLeft;   ///< The center-left edge.
    static const Anchor Center;       ///< The center.
    static const Anchor CenterRight;  ///< The center-right edge.
    static const Anchor BottomLeft;   ///< The bottom-left corner.
    static const Anchor BottomCenter; ///< The bottom-center edge.
    static const Anchor BottomRight;  ///< The bottom-right corner.

public:
    /// Create a top-left anchor.
    constexpr Anchor() noexcept = default;

    /// Create an anchor from low-level flags.
    /// If multiple flags select the same axis, the first one in left/center/right or top/center/bottom order is kept.
    /// @param value The flags that select the anchor components.
    constexpr Anchor(const AnchorFlags value) noexcept : // NOLINT(*-explicit-constructor)
        _value{makeExclusive(value)} {}

    // defaults
    ~Anchor() = default;
    Anchor(const Anchor &) = default;
    /// Assign an anchor.
    auto operator=(const Anchor &) -> Anchor & = default;

public: // operators
    /// Test two anchors for equality.
    auto operator==(const Anchor &) const noexcept -> bool = default;
    /// Test two anchors for inequality.
    auto operator!=(const Anchor &) const noexcept -> bool = default;
    /// Combine axis components from two anchors.
    /// Conflicting components are normalized using the same priority as the flag constructor.
    [[nodiscard]] friend constexpr auto operator|(Anchor lhs, Anchor rhs) noexcept -> Anchor {
        return Anchor{lhs._value | rhs._value};
    }

public: // tests
    /// Test whether this anchor selects the left edge.
    [[nodiscard]] constexpr auto isLeft() const noexcept -> bool {
        return horizontal().toRawValue() == AnchorFlag::Left;
    }
    /// Test whether this anchor selects the horizontal center.
    [[nodiscard]] constexpr auto isHorizontalCenter() const noexcept -> bool {
        return horizontal().toRawValue() == AnchorFlag::HCenter;
    }
    /// Test whether this anchor selects the right edge.
    [[nodiscard]] constexpr auto isRight() const noexcept -> bool {
        return horizontal().toRawValue() == AnchorFlag::Right;
    }
    /// Test whether this anchor selects the top edge.
    [[nodiscard]] constexpr auto isTop() const noexcept -> bool { return vertical().toRawValue() == AnchorFlag::Top; }
    /// Test whether this anchor selects the vertical center.
    [[nodiscard]] constexpr auto isVerticalCenter() const noexcept -> bool {
        return vertical().toRawValue() == AnchorFlag::VCenter;
    }
    /// Test whether this anchor selects the bottom edge.
    [[nodiscard]] constexpr auto isBottom() const noexcept -> bool {
        return vertical().toRawValue() == AnchorFlag::Bottom;
    }

public: // filter
    /// Return only the horizontal component.
    [[nodiscard]] constexpr auto horizontal() const noexcept -> Anchor {
        return Anchor{_value & AnchorFlag::HorizontalMask, PrivateTag{}};
    }
    /// Return only the vertical component.
    [[nodiscard]] constexpr auto vertical() const noexcept -> Anchor {
        return Anchor{_value & AnchorFlag::VerticalMask, PrivateTag{}};
    }

public: // conversion
    /// Create a hash value for this anchor.
    [[nodiscard]] auto hash() const noexcept -> std::size_t { return util::createHash(_value); }
    /// Return the normalized low-level flags.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> AnchorFlags { return _value; }

private:
    /// Create an anchor from normalized flags without normalization.
    /// @param value The normalized flags.
    explicit constexpr Anchor(const AnchorFlags value, PrivateTag) : _value{value} {}

    /// Normalize conflicting components by retaining the first selected value for each axis.
    /// @param rawFlags The flags to normalize.
    /// @return The normalized anchor flags.
    [[nodiscard]] constexpr static auto makeExclusive(AnchorFlags rawFlags) noexcept -> AnchorFlags {
        AnchorFlags result;
        if (rawFlags.isSet(AnchorFlag::Left)) {
            result |= AnchorFlag::Left;
        } else if (rawFlags.isSet(AnchorFlag::HCenter)) {
            result |= AnchorFlag::HCenter;
        } else if (rawFlags.isSet(AnchorFlag::Right)) {
            result |= AnchorFlag::Right;
        }
        if (rawFlags.isSet(AnchorFlag::Top)) {
            result |= AnchorFlag::Top;
        } else if (rawFlags.isSet(AnchorFlag::VCenter)) {
            result |= AnchorFlag::VCenter;
        } else if (rawFlags.isSet(AnchorFlag::Bottom)) {
            result |= AnchorFlag::Bottom;
        }
        return result;
    }

private:
    AnchorFlags _value{AnchorFlag::TopLeft}; ///< The normalized anchor flags.
};

inline constexpr Anchor Anchor::Left{AnchorFlag::Left};
inline constexpr Anchor Anchor::HCenter{AnchorFlag::HCenter};
inline constexpr Anchor Anchor::Right{AnchorFlag::Right};
inline constexpr Anchor Anchor::Top{AnchorFlag::Top};
inline constexpr Anchor Anchor::VCenter{AnchorFlag::VCenter};
inline constexpr Anchor Anchor::Bottom{AnchorFlag::Bottom};
inline constexpr Anchor Anchor::TopLeft{AnchorFlag::TopLeft};
inline constexpr Anchor Anchor::TopCenter{AnchorFlag::TopCenter};
inline constexpr Anchor Anchor::TopRight{AnchorFlag::TopRight};
inline constexpr Anchor Anchor::CenterLeft{AnchorFlag::CenterLeft};
inline constexpr Anchor Anchor::Center{AnchorFlag::Center};
inline constexpr Anchor Anchor::CenterRight{AnchorFlag::CenterRight};
inline constexpr Anchor Anchor::BottomLeft{AnchorFlag::BottomLeft};
inline constexpr Anchor Anchor::BottomCenter{AnchorFlag::BottomCenter};
inline constexpr Anchor Anchor::BottomRight{AnchorFlag::BottomRight};

}

template <>
struct std::hash<erbsland::geometry::Anchor> {
    auto operator()(const erbsland::geometry::Anchor &anchor) const noexcept -> std::size_t { return anchor.hash(); }
};
