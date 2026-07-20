// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CharCompareFn.hpp"
#include "String_fwd.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>

namespace erbsland::text {

/// The case sensitivity used for character-wise text comparisons.
/// @seedoc{/reference/text/char_range}
/// @tested{CaseSensitivityTest}
class CaseSensitivity final {
public:
    /// The case sensitivity value.
    enum Value : uint8_t {
        CaseSensitive = 0, ///< Compare exact Unicode code-point values.
        CaseInsensitive,   ///< Compare case-folded Unicode code-point values.
    };

public:
    /// Create a case sensitivity from a value.
    constexpr CaseSensitivity(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr CaseSensitivity() noexcept = default;
    ~CaseSensitivity() = default;
    CaseSensitivity(const CaseSensitivity &) = default;
    CaseSensitivity(CaseSensitivity &&) = default;
    auto operator=(const CaseSensitivity &) -> CaseSensitivity & = default;
    auto operator=(CaseSensitivity &&) -> CaseSensitivity & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const CaseSensitivity &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const CaseSensitivity &other, value, other._value);

public: // accessors
    /// Get the raw case sensitivity value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }
    /// Get the character comparison callback using Unicode case folding.
    /// For case-sensitive comparisons, this returns an empty callback to select exact code-point comparison.
    [[nodiscard]] auto comparisonFn() const noexcept -> CharCompareFn;
    /// Get the character comparison callback using ASCII-only case folding.
    /// For case-sensitive comparisons, this returns an empty callback to select exact code-point comparison.
    [[nodiscard]] auto asciiComparisonFn() const noexcept -> CharCompareFn;

public: // conversion
    /// Convert this case sensitivity to its canonical name.
    [[nodiscard]] auto toString() const noexcept -> String;

private:
    Value _value{CaseSensitive}; ///< The case sensitivity value.
};

/// Convenient constant for case-sensitive text comparison.
inline constexpr auto cCaseSensitive = CaseSensitivity{CaseSensitivity::CaseSensitive};
/// Convenient constant for case-insensitive text comparison.
inline constexpr auto cCaseInsensitive = CaseSensitivity{CaseSensitivity::CaseInsensitive};

}
