// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/EnumFlagsTraits.hpp"

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <type_traits>

namespace erbsland::util {

/// A safe value wrapper for scoped enum flags.
/// @seedoc{/reference/util/supporting_utilities}
/// @tparam tEnum The scoped enum type with unsigned underlying type.
/// @tested{EnumFlagsTest}
template <impl::EnumFlagsEnum tEnum>
class EnumFlags final {
    struct RawValueTag {};

public:
    /// The enum type used for the individual flags.
    using Enum = tEnum;
    /// The unsigned integer type used to store the raw flag bits.
    using Value = typename impl::EnumFlagsTraits<tEnum>::Value;

public:
    /// Create an empty flag set.
    constexpr EnumFlags() noexcept = default;
    /// Create a flag set with one enum flag.
    /// @param flag The flag to set.
    constexpr EnumFlags(tEnum flag) noexcept : _value{impl::enumFlagsRawValue(flag)} {} // NOLINT(*-explicit*)
    /// Create a flag set from a list of enum flags.
    /// @param flags The flags to set.
    constexpr EnumFlags(std::initializer_list<tEnum> flags) noexcept {
        for (const auto flag : flags) {
            set(flag);
        }
    }

    // defaults
    ~EnumFlags() = default;
    EnumFlags(const EnumFlags &) noexcept = default;
    auto operator=(const EnumFlags &) noexcept -> EnumFlags & = default;
    EnumFlags(EnumFlags &&) noexcept = default;
    auto operator=(EnumFlags &&) noexcept -> EnumFlags & = default;

public: // operators
    /// Test if two flag sets store the same raw bits.
    [[nodiscard]] constexpr auto operator==(const EnumFlags &other) const noexcept -> bool = default;
    /// Calculate the intersection of this flag set and another flag set.
    /// @param other The other flag set.
    /// @return The flags common to both sets.
    [[nodiscard]] constexpr auto operator&(EnumFlags other) const noexcept -> EnumFlags {
        return fromRawValue(_value & other._value);
    }
    /// Calculate the intersection of this flag set and an enum flag.
    /// @param flag The flag to intersect with.
    /// @return The flags common to both sets.
    [[nodiscard]] constexpr auto operator&(tEnum flag) const noexcept -> EnumFlags {
        return fromRawValue(_value & impl::enumFlagsRawValue(flag));
    }
    /// Keep only the bits also set in another flag set.
    /// @param other The flag set to intersect with.
    /// @return A reference to this flag set.
    constexpr auto operator&=(EnumFlags other) noexcept -> EnumFlags & {
        _value &= other._value;
        return *this;
    }
    /// Keep only the bits also set in an enum flag.
    /// @param flag The flag to intersect with.
    /// @return A reference to this flag set.
    constexpr auto operator&=(tEnum flag) noexcept -> EnumFlags & {
        _value &= impl::enumFlagsRawValue(flag);
        return *this;
    }
    /// Calculate the union of this flag set and another flag set.
    /// @param other The other flag set.
    /// @return The combined flags.
    [[nodiscard]] constexpr auto operator|(EnumFlags other) const noexcept -> EnumFlags {
        return fromRawValue(_value | other._value);
    }
    /// Calculate the union of this flag set and an enum flag.
    /// @param flag The flag to add.
    /// @return The combined flags.
    [[nodiscard]] constexpr auto operator|(tEnum flag) const noexcept -> EnumFlags {
        return fromRawValue(_value | impl::enumFlagsRawValue(flag));
    }
    /// Add all bits from another flag set.
    /// @param other The flag set to add.
    /// @return A reference to this flag set.
    constexpr auto operator|=(EnumFlags other) noexcept -> EnumFlags & {
        _value |= other._value;
        return *this;
    }
    /// Add all bits from an enum flag.
    /// @param flag The flag to add.
    /// @return A reference to this flag set.
    constexpr auto operator|=(tEnum flag) noexcept -> EnumFlags & {
        _value |= impl::enumFlagsRawValue(flag);
        return *this;
    }
    /// Calculate the exclusive union of this flag set and another flag set.
    /// @param other The other flag set.
    /// @return The flags set in exactly one of the two sets.
    [[nodiscard]] constexpr auto operator^(EnumFlags other) const noexcept -> EnumFlags {
        return fromRawValue(_value ^ other._value);
    }
    /// Calculate the exclusive union of this flag set and an enum flag.
    /// @param flag The flag to toggle.
    /// @return The toggled flags.
    [[nodiscard]] constexpr auto operator^(tEnum flag) const noexcept -> EnumFlags {
        return fromRawValue(_value ^ impl::enumFlagsRawValue(flag));
    }
    /// Toggle all bits from another flag set.
    /// @param other The flag set to toggle.
    /// @return A reference to this flag set.
    constexpr auto operator^=(EnumFlags other) noexcept -> EnumFlags & {
        _value ^= other._value;
        return *this;
    }
    /// Toggle all bits from an enum flag.
    /// @param flag The flag to toggle.
    /// @return A reference to this flag set.
    constexpr auto operator^=(tEnum flag) noexcept -> EnumFlags & {
        _value ^= impl::enumFlagsRawValue(flag);
        return *this;
    }
    /// Calculate the bounded complement of this flag set using `tEnum::All` as the valid bit domain.
    /// @return The inverted flags within the valid domain.
    [[nodiscard]] constexpr auto operator~() const noexcept -> EnumFlags
        requires impl::EnumFlagsEnumWithAll<tEnum>
    {
        return fromRawValue(impl::enumFlagsRawValue(tEnum::All) & invertedValue(_value));
    }
    /// Calculate the intersection of an enum flag and a flag set.
    [[nodiscard]] friend constexpr auto operator&(tEnum flag, EnumFlags flags) noexcept -> EnumFlags {
        return flags & flag;
    }
    /// Calculate the union of an enum flag and a flag set.
    [[nodiscard]] friend constexpr auto operator|(tEnum flag, EnumFlags flags) noexcept -> EnumFlags {
        return flags | flag;
    }
    /// Calculate the exclusive union of an enum flag and a flag set.
    [[nodiscard]] friend constexpr auto operator^(tEnum flag, EnumFlags flags) noexcept -> EnumFlags {
        return flags ^ flag;
    }

public: // accessors
    /// Test if this flag set is empty.
    /// @return `true` if no flags are set.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _value == 0U; }
    /// Test if this flag set contains at least one raw bit.
    /// @return `true` if any flag is set.
    [[nodiscard]] constexpr auto hasAny() const noexcept -> bool { return _value != 0U; }
    /// Test if all bits of an enum flag are set.
    /// @param flag The flag to check.
    /// @return `true` if the flag is set.
    [[nodiscard]] constexpr auto isSet(tEnum flag) const noexcept -> bool {
        const auto flagValue = impl::enumFlagsRawValue(flag);
        return flagValue != 0U && (_value & flagValue) == flagValue;
    }
    /// Test if all bits from another flag set are set.
    /// @param flags The flags to check.
    /// @return `true` if all specified flags are set.
    [[nodiscard]] constexpr auto contains(EnumFlags flags) const noexcept -> bool {
        return (_value & flags._value) == flags._value;
    }
    /// Test if at least one bit from another flag set is set.
    /// @param flags The flags to check.
    /// @return `true` if any of the specified flags are set.
    [[nodiscard]] constexpr auto intersects(EnumFlags flags) const noexcept -> bool {
        return (_value & flags._value) != 0U;
    }

public:
    /// Set all bits from an enum flag.
    /// @param flag The flag to set.
    constexpr void set(tEnum flag) noexcept { _value |= impl::enumFlagsRawValue(flag); }
    /// Set all bits from another flag set.
    /// @param flags The flags to set.
    constexpr void set(EnumFlags flags) noexcept { _value |= flags._value; }
    /// Clear all bits from an enum flag.
    /// @param flag The flag to clear.
    constexpr void clear(tEnum flag) noexcept { _value &= invertedValue(impl::enumFlagsRawValue(flag)); }
    /// Clear all bits from another flag set.
    /// @param flags The flags to clear.
    constexpr void clear(EnumFlags flags) noexcept { _value &= invertedValue(flags._value); }
    /// Clear all bits.
    constexpr void clear() noexcept { _value = 0U; }
    /// Replace only the bits selected by `mask` with the corresponding bits from `flags`.
    /// @param flags The new flag bits.
    /// @param mask The mask selecting which bits to replace.
    constexpr void replaceMasked(EnumFlags flags, EnumFlags mask) noexcept {
        _value = (_value & invertedValue(mask._value)) | (flags._value & mask._value);
    }

public: // conversion
    /// Create a flag set from raw flag bits.
    /// @param value The raw flag bits.
    /// @return A new flag set with the given bits.
    [[nodiscard]] constexpr static auto fromRawValue(Value value) noexcept -> EnumFlags {
        return EnumFlags{value, RawValueTag{}};
    }
    /// Return the raw flag bits.
    /// @return The underlying integer value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

private:
    constexpr explicit EnumFlags(Value value, RawValueTag) noexcept : _value{value} {}
    [[nodiscard]] constexpr static auto invertedValue(Value value) noexcept -> Value {
        return static_cast<Value>(~value);
    }

private:
    Value _value{0U}; ///< The stored raw flag bits.
};

}

namespace std {

template <erbsland::util::impl::EnumFlagsEnum tEnum>
struct hash<erbsland::util::EnumFlags<tEnum>> {
    auto operator()(const erbsland::util::EnumFlags<tEnum> &value) const noexcept -> std::size_t {
        return std::hash<typename erbsland::util::EnumFlags<tEnum>::Value>{}(value.toRawValue());
    }
};

}
