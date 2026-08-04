// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <unordered_map>

namespace erbsland::conf::impl {

/// The dependency mode for a dependency definition.
class DependencyMode {
public:
    enum Enum : uint8_t {
        // Individual building blocks
        AllowNone = 1U << 0U,        ///< Neither sources nor targets are configured.
        AllowOnlyTargets = 1U << 1U, ///< Only the targets are configured.
        AllowOnlySources = 1U << 2U, ///< Only the sources are configured.
        AllowBoth = 1U << 3U,        ///< Both sources and targets are configured.

        /// Undefined dependency mode.
        Undefined = 0,
        /// If the sources are configured, targets must also be configured.
        If = AllowNone | AllowOnlyTargets | AllowBoth,
        /// If the sources are not configured, targets must not be configured.
        IfNot = AllowNone | AllowOnlyTargets | AllowOnlySources,
        /// At least one of the sources or targets must be configured.
        OR = AllowOnlyTargets | AllowOnlySources | AllowBoth,
        /// Either the sources or the targets are configured. Not both and not none.
        XOR = AllowOnlyTargets | AllowOnlySources,
        /// Either all sources and all targets are configured, or none of them are.
        XNOR = AllowNone | AllowBoth,
        /// Both sources and targets must be configured.
        AND = AllowBoth,

        // not used modes
        NoRestriction = AllowNone | AllowOnlySources | AllowOnlyTargets | AllowBoth,
        NAND = IfNot,
        NOR = AllowNone,
    };

public:
    /// Create an undefined dependency mode.
    DependencyMode() = default;
    /// Create a dependency mode from an enum value.
    /// @param value The enum value.
    DependencyMode(const Enum value) : _value(value) {} // NOLINT(*-explicit-constructor)

    // defaults
    /// Default copy constructor.
    DependencyMode(const DependencyMode &) = default;
    /// Default destructor.
    ~DependencyMode() = default;
    /// Default copy assignment.
    auto operator=(const DependencyMode &) -> DependencyMode & = default;

public: // operators
    /// Assign an enum value.
    auto operator=(const Enum value) noexcept -> DependencyMode & {
        _value = value;
        return *this;
    }

public: // comparison
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const DependencyMode &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, Enum value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(Enum a, const DependencyMode &b, a, b._value);

public: // tests
    /// Test if the dependency situation is valid for this mode.
    /// @param hasSource If one of the source values exists.
    /// @param hasTarget If one of the target values exists.
    /// @return `true` if the dependency situation is valid for this mode.
    [[nodiscard]] auto isValid(bool hasSource, bool hasTarget) const noexcept -> bool;

public: // conversion
    /// Convert this dependency mode into text.
    /// For all unsupported enum values, this method returns `undefined`.
    [[nodiscard]] auto toText() const noexcept -> const text::String &;

    /// Parse a dependency mode from text.
    /// The parsing is case-insensitive and treats spaces and underscores equally.
    /// @param text The input text.
    /// @return The parsed dependency mode, or `Undefined` if the input text is not supported.
    [[nodiscard]] static auto fromText(const text::String &text) noexcept -> DependencyMode;

public:
    /// Access the underlying enum value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Enum { return _value; }

private:
    using TextToValueMap = std::vector<std::pair<text::String, Enum>>;
    /// Get the static mapping between normalized text and enum values.
    [[nodiscard]] static auto textToValueMap() noexcept -> const TextToValueMap &;

private:
    Enum _value{Undefined};
};

}
