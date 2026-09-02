// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AxisMappable.hpp"
#include "SignedAxisMapping.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

namespace erbsland::geometry {

/// Applies an exact axis permutation and direction mapping to geometry values.
/// @seedoc{/reference/geometry/geometry}
/// @tested{AxisMapperTest}
class AxisMapper final {
public:
    /// Create a two-dimensional mapper with the specified main-axis orientation.
    /// @param orientation The orientation of the main axis.
    constexpr explicit AxisMapper(const Orientation orientation) noexcept : _mapping{AxisMapping{orientation}} {}
    /// Create a mapper from an unsigned axis mapping.
    /// @param mapping The unsigned mapping to use.
    constexpr explicit AxisMapper(const AxisMapping mapping) : _mapping{mapping} {}
    /// Create a mapper from a signed axis mapping.
    /// @param mapping The signed mapping to use.
    constexpr explicit AxisMapper(const SignedAxisMapping mapping) noexcept : _mapping{mapping} {}

public: // attributes
    /// Get the dimensionality required by this mapper.
    [[nodiscard]] constexpr auto dimensionality() const noexcept -> Dimensionality { return _mapping.dimensionality(); }
    /// Get the canonical signed mapping.
    [[nodiscard]] constexpr auto mapping() const noexcept -> const SignedAxisMapping & { return _mapping; }

public: // mapping
    /// Map the axes of a geometry value.
    /// @tparam T The geometry value type.
    /// @param value The value to map.
    /// @return The reconstructed mapped value.
    /// @throws err::ParameterError if the value and mapping dimensionalities differ.
    template <AxisMappable T>
    [[nodiscard]] constexpr auto map(T value) const -> T {
        using namespace text::literals;
        if (T::cDimensionality != dimensionality()) {
            throw err::ParameterError{
                "The geometry value and axis mapping have different dimensionalities."_el, "value"_el};
        }
        if (_mapping.isIdentity()) {
            return value;
        }
        if constexpr (T::cDimensionality == Dimensionality::One) {
            return T{value.component(_mapping.source(Axis::X))};
        } else if constexpr (T::cDimensionality == Dimensionality::Two) {
            return T{
                value.component(_mapping.source(Axis::X)),
                value.component(_mapping.source(Axis::Y)),
            };
        } else {
            return T{
                value.component(_mapping.source(Axis::X)),
                value.component(_mapping.source(Axis::Y)),
                value.component(_mapping.source(Axis::Z)),
            };
        }
    }

private:
    SignedAxisMapping _mapping; ///< The canonical signed mapping.
};

}
