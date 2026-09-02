// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <vector>

namespace erbsland::conf::vr {

/// The relationship enforced between source and target values of a dependency.
/// @tested{VrBuilderApiTest VrDependenciesTest}
class DependencyMode {
public:
    enum Enum : uint8_t {
        AllowNone = 1U << 0U,                                    ///< Neither sources nor targets are configured.
        AllowOnlyTargets = 1U << 1U,                             ///< Only the targets are configured.
        AllowOnlySources = 1U << 2U,                             ///< Only the sources are configured.
        AllowBoth = 1U << 3U,                                    ///< Both sources and targets are configured.

        Undefined = 0,                                           ///< Undefined dependency mode.
        If = AllowNone | AllowOnlyTargets | AllowBoth,           ///< Sources require targets.
        IfNot = AllowNone | AllowOnlyTargets | AllowOnlySources, ///< Sources exclude targets.
        OR = AllowOnlyTargets | AllowOnlySources | AllowBoth,    ///< At least one side must be configured.
        XOR = AllowOnlyTargets | AllowOnlySources,               ///< Exactly one side must be configured.
        XNOR = AllowNone | AllowBoth,                            ///< Both sides or neither side must be configured.
        AND = AllowBoth,                                         ///< Both sides must be configured.

        NoRestriction = AllowNone | AllowOnlySources | AllowOnlyTargets | AllowBoth, ///< All states are valid.
        NAND = IfNot,                                                                ///< Both sides must not coexist.
        NOR = AllowNone, ///< Neither side may be configured.
    };

public:
    /// Create an undefined dependency mode.
    DependencyMode() = default;
    /// Create a dependency mode from an enum value.
    /// @param value The enum value.
    DependencyMode(const Enum value) : _value(value) {} // NOLINT(*-explicit-constructor)

    // defaults
    DependencyMode(const DependencyMode &) = default;
    ~DependencyMode() = default;
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
    /// Test whether a source/target presence combination satisfies this mode.
    /// @param hasSource `true` if at least one source value exists.
    /// @param hasTarget `true` if at least one target value exists.
    /// @return `true` if the combination is accepted.
    [[nodiscard]] auto isValid(bool hasSource, bool hasTarget) const noexcept -> bool;

public: // conversion
    /// Convert this mode into its ELCL identifier.
    [[nodiscard]] auto toText() const noexcept -> const text::String &;
    /// Parse an ELCL dependency-mode identifier.
    /// @param text The identifier to parse.
    /// @return The parsed mode, or `Undefined` for unsupported input.
    [[nodiscard]] static auto fromText(const text::String &text) noexcept -> DependencyMode;
    /// Access the underlying enum value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Enum { return _value; }

private:
    using TextToValueMap = std::vector<std::pair<text::String, Enum>>;
    /// Get the static mapping between normalized text and enum values.
    [[nodiscard]] static auto textToValueMap() noexcept -> const TextToValueMap &;

private:
    Enum _value{Undefined}; ///< The selected mode.
};

}
