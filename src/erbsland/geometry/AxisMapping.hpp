// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Axis.hpp"
#include "Dimensionality.hpp"
#include "Orientation.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <array>
#include <cstddef>

namespace erbsland::geometry {

/// An exact permutation of one, two, or three physical axes.
/// Constructor arguments specify the source axis used for each destination axis in x/y/z order.
/// @seedoc{/reference/geometry/geometry}
/// @tested{AxisMapperTest}
class AxisMapping final {
public:
    /// Create a one-dimensional mapping.
    /// @param x The source axis for destination X.
    /// @throws err::ParameterError if `x` is not X.
    constexpr explicit AxisMapping(Axis x) : _sources{x, Axis::Y, Axis::Z}, _dimensionality{Dimensionality::One} {
        validate();
    }
    /// Create a two-dimensional mapping.
    /// @param x The source axis for destination X.
    /// @param y The source axis for destination Y.
    /// @throws err::ParameterError if the axes are not an exact X/Y permutation.
    constexpr AxisMapping(Axis x, Axis y) : _sources{x, y, Axis::Z}, _dimensionality{Dimensionality::Two} {
        validate();
    }
    /// Create a three-dimensional mapping.
    /// @param x The source axis for destination X.
    /// @param y The source axis for destination Y.
    /// @param z The source axis for destination Z.
    /// @throws err::ParameterError if the axes are not an exact X/Y/Z permutation.
    constexpr AxisMapping(Axis x, Axis y, Axis z) : _sources{x, y, z}, _dimensionality{Dimensionality::Three} {
        validate();
    }
    /// Create a two-dimensional mapping from a main-axis orientation.
    /// Horizontal orientation creates the identity; vertical orientation exchanges X and Y.
    /// @param orientation The main-axis orientation.
    constexpr explicit AxisMapping(const Orientation orientation) noexcept :
        _sources{
            orientation == Orientation::Horizontal ? Axis::X : Axis::Y,
            orientation == Orientation::Horizontal ? Axis::Y : Axis::X,
            Axis::Z},
        _dimensionality{Dimensionality::Two} {}

public: // operators
    /// Compare two mappings.
    auto operator==(const AxisMapping &) const noexcept -> bool = default;
    /// Compare two mappings.
    auto operator!=(const AxisMapping &) const noexcept -> bool = default;

public: // tests
    /// Test whether this mapping preserves every active axis.
    [[nodiscard]] constexpr auto isIdentity() const noexcept -> bool {
        const auto count = static_cast<std::size_t>(_dimensionality);
        for (auto index = std::size_t{0}; index < count; ++index) {
            if (_sources[index] != static_cast<Axis>(index)) {
                return false;
            }
        }
        return true;
    }

public: // attributes
    /// Get the dimensionality declared by this mapping.
    [[nodiscard]] constexpr auto dimensionality() const noexcept -> Dimensionality { return _dimensionality; }
    /// Get the source axis used for a destination axis.
    /// @param destination The destination axis to inspect.
    /// @return The source axis for `destination`.
    /// @throws err::ParameterError if `destination` is outside this mapping's dimensionality.
    [[nodiscard]] constexpr auto source(const Axis destination) const -> Axis {
        using namespace text::literals;
        const auto index = static_cast<std::size_t>(destination);
        if (index >= static_cast<std::size_t>(_dimensionality)) {
            throw err::ParameterError{
                "The destination axis is outside the mapping dimensionality."_el, "destination"_el};
        }
        return _sources[index];
    }

private:
    /// Validate that the active sources form an exact permutation.
    /// @throws err::ParameterError if the mapping is invalid.
    constexpr void validate() const {
        using namespace text::literals;
        const auto count = static_cast<std::size_t>(_dimensionality);
        for (auto index = std::size_t{0}; index < count; ++index) {
            const auto sourceIndex = static_cast<std::size_t>(_sources[index]);
            if (sourceIndex >= count) {
                throw err::ParameterError{"A source axis is outside the mapping dimensionality."_el, "axes"_el};
            }
            for (auto previous = std::size_t{0}; previous < index; ++previous) {
                if (_sources[previous] == _sources[index]) {
                    throw err::ParameterError{"Each source axis must occur exactly once."_el, "axes"_el};
                }
            }
        }
    }

private:
    std::array<Axis, 3> _sources;   ///< Source axes in destination order.
    Dimensionality _dimensionality; ///< The active dimensionality.
};

}
