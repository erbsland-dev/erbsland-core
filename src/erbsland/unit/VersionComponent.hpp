// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "VersionComponent.hpp"
#include "VersionComponent_fwd.hpp"

#include "impl/Throw.hpp"

#include "../math/IntegerConversion.hpp"
#include "../math/SaturatingMath.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <cstdint>
#include <functional>

namespace erbsland::unit {

/// A distinct unsigned 16-bit version component with saturating count arithmetic.
/// @tparam tPart The version field represented by this type.
/// @seedoc{/reference/unit/units_and_versions}
/// @tested{VersionTest}
template <VersionPart tPart>
class VersionComponent {
public:
    /// Tag for unchecked construction.
    struct Unchecked {};
    /// The raw component representation.
    using Value = uint16_t;
    /// The version field.
    static constexpr auto cPart = tPart;
    /// The greatest component value.
    static constexpr Value cRawMaximum = std::numeric_limits<Value>::max();

public:
    /// Create a zero version component.
    constexpr VersionComponent() noexcept = default;
    /// Construct a component from a compile-time integer in 0...65535.
    /// @param value The component value; invalid values fail compilation.
    template <math::NativeInteger T>
    explicit consteval VersionComponent(T value) : _value{checkedInteger<T>(value)} {}
    /// Construct a component unchecked.
    constexpr VersionComponent(const Value value, Unchecked) noexcept : _value{value} {}

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const VersionComponent &other, other._value);
    /// Add an unsigned count with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator+(T count) const noexcept -> VersionComponent {
        return added(count);
    }
    /// Add an unsigned count in place with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator+=(T count) noexcept -> VersionComponent & {
        return add(count);
    }
    /// Subtract an unsigned count with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto operator-(T count) const noexcept -> VersionComponent {
        return subtracted(count);
    }
    /// Subtract an unsigned count in place with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto operator-=(T count) noexcept -> VersionComponent & {
        return subtract(count);
    }
    /// Increment with saturation.
    constexpr auto operator++() noexcept -> VersionComponent & { return increment(); }
    /// Increment with saturation and return the previous component.
    constexpr auto operator++(int) noexcept -> VersionComponent {
        auto previous = *this;
        increment();
        return previous;
    }
    /// Decrement with saturation.
    constexpr auto operator--() noexcept -> VersionComponent & { return decrement(); }
    /// Decrement with saturation and return the previous component.
    constexpr auto operator--(int) noexcept -> VersionComponent {
        auto previous = *this;
        decrement();
        return previous;
    }

public: // tests
    /// Test whether the component is zero.
    [[nodiscard]] constexpr auto isZero() const noexcept -> bool { return _value == 0; }
    /// Test whether the component is one.
    [[nodiscard]] constexpr auto isOne() const noexcept -> bool { return _value == 1; }
    /// Test whether the component is minimum.
    [[nodiscard]] constexpr auto isMinimum() const noexcept -> bool { return _value == 0; }
    /// Test whether the component is maximum.
    [[nodiscard]] constexpr auto isMaximum() const noexcept -> bool { return _value == cRawMaximum; }

public: // accessors
    /// Get the native component value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // math
    /// Add an unsigned count with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto add(T count) noexcept -> VersionComponent & {
        *this = added(count);
        return *this;
    }
    /// Return the component after adding an unsigned count with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto added(T count) const noexcept -> VersionComponent {
        const auto raw = math::toNativeInteger(count);
        if (raw > static_cast<uint64_t>(cRawMaximum - _value)) {
            return maximum();
        }
        return fromRaw(static_cast<Value>(_value + raw));
    }
    /// Add an unsigned count exactly.
    /// @throws err::OverflowError if the result is outside 0...65535.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto addOrThrow(T count) -> VersionComponent & {
        *this = addedOrThrow(count);
        return *this;
    }
    /// Return the component after exact count arithmetic.
    /// @throws err::OverflowError if the result is outside 0...65535.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto addedOrThrow(T count) const -> VersionComponent {
        if (math::toNativeInteger(count) > static_cast<uint64_t>(cRawMaximum - _value)) {
            impl::throwOverflow("Version component arithmetic exceeds bounds");
        }
        return added(count);
    }
    /// Subtract an unsigned count with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto subtract(T count) noexcept -> VersionComponent & {
        *this = subtracted(count);
        return *this;
    }
    /// Return the component after subtracting an unsigned count with saturation.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto subtracted(T count) const noexcept -> VersionComponent {
        const auto raw = math::toNativeInteger(count);
        if (raw > static_cast<uint64_t>(_value)) {
            return zero();
        }
        return fromRaw(static_cast<Value>(_value - raw));
    }
    /// Subtract an unsigned count exactly.
    /// @throws err::OverflowError if the result is outside 0...65535.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    constexpr auto subtractOrThrow(T count) -> VersionComponent & {
        *this = subtractedOrThrow(count);
        return *this;
    }
    /// Return the component after exact count arithmetic.
    /// @throws err::OverflowError if the result is outside 0...65535.
    template <math::AnyIntegerType T>
        requires std::unsigned_integral<math::NativeIntegerOfT<T>>
    [[nodiscard]] constexpr auto subtractedOrThrow(T count) const -> VersionComponent {
        if (math::toNativeInteger(count) > static_cast<uint64_t>(_value)) {
            impl::throwOverflow("Version component arithmetic exceeds bounds");
        }
        return subtracted(count);
    }
    /// Increment with saturation.
    constexpr auto increment() noexcept -> VersionComponent & { return add(1U); }
    /// Return the component incremented with saturation.
    [[nodiscard]] constexpr auto incremented() const noexcept -> VersionComponent { return added(1U); }
    /// Decrement with saturation.
    constexpr auto decrement() noexcept -> VersionComponent & { return subtract(1U); }
    /// Return the component decremented with saturation.
    [[nodiscard]] constexpr auto decremented() const noexcept -> VersionComponent { return subtracted(1U); }

public: // factories
    /// Return the zero component.
    [[nodiscard]] static constexpr auto zero() noexcept -> VersionComponent { return fromRaw(0); }
    /// Return the one component.
    [[nodiscard]] static constexpr auto one() noexcept -> VersionComponent { return fromRaw(1); }
    /// Return the minimum component.
    [[nodiscard]] static constexpr auto minimum() noexcept -> VersionComponent { return fromRaw(0); }
    /// Return the maximum component.
    [[nodiscard]] static constexpr auto maximum() noexcept -> VersionComponent { return fromRaw(cRawMaximum); }
    /// Convert a runtime integer with saturation to 0...65535.
    template <math::NativeInteger T>
    [[nodiscard]] static constexpr auto fromInteger(T value) noexcept -> VersionComponent {
        return VersionComponent{math::saturatingCast<Value>(value), Unchecked{}};
    }
    /// Convert a runtime integer exactly.
    /// @throws err::OverflowError if value is outside 0...65535.
    template <math::NativeInteger T>
    [[nodiscard]] static constexpr auto fromIntegerOrThrow(T value) -> VersionComponent {
        return fromRaw(checkedInteger<T>(value));
    }

private:
    /// Check if an integer is in the range of this unit.
    template <math::NativeInteger T>
    [[nodiscard]] constexpr static auto checkedInteger(T value) -> Value {
        if (math::willCastOverflow<Value>(value)) {
            impl::throwOverflow("Integer is outside the unit's finite range");
        }
        return static_cast<Value>(value);
    }
    /// Store a value whose bounds have already been checked.
    [[nodiscard]] static constexpr auto fromRaw(Value value) noexcept -> VersionComponent {
        auto result = VersionComponent{};
        result._value = value;
        return result;
    }

    Value _value{0}; ///< The component value.
};

}

template <erbsland::unit::VersionPart tPart>
struct std::hash<erbsland::unit::VersionComponent<tPart>> {
    auto operator()(erbsland::unit::VersionComponent<tPart> value) const noexcept -> std::size_t {
        return std::hash<uint16_t>{}(value.toRawValue());
    }
};
