// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AxisMapping.hpp"
#include "SignedAxis.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <array>
#include <cstddef>

namespace erbsland::geometry {

/// An exact axis permutation with independent direction reversal for every destination axis.
/// Constructor arguments specify the signed source axis used for each destination axis in x/y/z order.
/// @seedoc{/reference/geometry/geometry}
/// @tested{AxisMapperTest}
class SignedAxisMapping final {
public:
    /// Create a one-dimensional signed mapping.
    /// @param x The signed source axis for destination X.
    /// @throws err::ParameterError if `x` does not select X.
    constexpr explicit SignedAxisMapping(SignedAxis x) :
        _sources{x, SignedAxis::PositiveY, SignedAxis::PositiveZ}, _dimensionality{Dimensionality::One} {
        validate();
    }
    /// Create a two-dimensional signed mapping.
    /// @param x The signed source axis for destination X.
    /// @param y The signed source axis for destination Y.
    /// @throws err::ParameterError if the axes are not an exact X/Y permutation.
    constexpr SignedAxisMapping(SignedAxis x, SignedAxis y) :
        _sources{x, y, SignedAxis::PositiveZ}, _dimensionality{Dimensionality::Two} {
        validate();
    }
    /// Create a three-dimensional signed mapping.
    /// @param x The signed source axis for destination X.
    /// @param y The signed source axis for destination Y.
    /// @param z The signed source axis for destination Z.
    /// @throws err::ParameterError if the axes are not an exact X/Y/Z permutation.
    constexpr SignedAxisMapping(SignedAxis x, SignedAxis y, SignedAxis z) :
        _sources{x, y, z}, _dimensionality{Dimensionality::Three} {
        validate();
    }
    /// Create an all-positive signed mapping from an unsigned mapping.
    /// @param mapping The source mapping.
    constexpr SignedAxisMapping(const AxisMapping mapping) : _dimensionality{mapping.dimensionality()} {
        const auto count = static_cast<std::size_t>(_dimensionality);
        for (auto index = std::size_t{0}; index < count; ++index) {
            _sources[index] = SignedAxis{mapping.source(static_cast<Axis>(index))};
        }
    }

public: // operators
    /// Compare two mappings.
    auto operator==(const SignedAxisMapping &) const noexcept -> bool = default;
    /// Compare two mappings.
    auto operator!=(const SignedAxisMapping &) const noexcept -> bool = default;

public: // tests
    /// Test whether this mapping preserves every active axis and direction.
    [[nodiscard]] constexpr auto isIdentity() const noexcept -> bool {
        const auto count = static_cast<std::size_t>(_dimensionality);
        for (auto index = std::size_t{0}; index < count; ++index) {
            if (_sources[index].isReversed() || _sources[index].axis() != static_cast<Axis>(index)) {
                return false;
            }
        }
        return true;
    }

public: // attributes
    /// Get the dimensionality declared by this mapping.
    [[nodiscard]] constexpr auto dimensionality() const noexcept -> Dimensionality { return _dimensionality; }
    /// Get the signed source axis used for a destination axis.
    /// @param destination The destination axis to inspect.
    /// @return The signed source axis for `destination`.
    /// @throws err::ParameterError if `destination` is outside this mapping's dimensionality.
    [[nodiscard]] constexpr auto source(const Axis destination) const -> SignedAxis {
        using namespace text::literals;
        const auto index = static_cast<std::size_t>(destination);
        if (index >= static_cast<std::size_t>(_dimensionality)) {
            throw err::ParameterError{
                "The destination axis is outside the mapping dimensionality."_el, "destination"_el};
        }
        return _sources[index];
    }

private:
    /// Validate that the active sources form an exact signed permutation.
    /// @throws err::ParameterError if the mapping is invalid.
    constexpr void validate() const {
        using namespace text::literals;
        const auto count = static_cast<std::size_t>(_dimensionality);
        for (auto index = std::size_t{0}; index < count; ++index) {
            if (!_sources[index].isValid() || static_cast<std::size_t>(_sources[index].axis()) >= count) {
                throw err::ParameterError{"A source axis is outside the mapping dimensionality."_el, "axes"_el};
            }
            for (auto previous = std::size_t{0}; previous < index; ++previous) {
                if (_sources[previous].axis() == _sources[index].axis()) {
                    throw err::ParameterError{"Each source axis must occur exactly once."_el, "axes"_el};
                }
            }
        }
    }

private:
    std::array<SignedAxis, 3> _sources; ///< Signed source axes in destination order.
    Dimensionality _dimensionality;     ///< The active dimensionality.
};

}
