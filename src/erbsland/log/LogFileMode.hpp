// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String_fwd.hpp"
#include "../text/StringList_fwd.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::log {

/// The mode used when initially opening a log file.
/// @tested{LogCoreTest}
class LogFileMode final {
public:
    /// The raw file mode value.
    enum Value : uint8_t {
        Overwrite, ///< Replace existing file content when first opened.
        Append,    ///< Preserve existing file content when first opened.
    };

    /// Create the default append mode.
    constexpr LogFileMode() noexcept = default;
    /// Create a mode from its raw value.
    /// @param value The raw file mode value.
    constexpr LogFileMode(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

public:                                                                  // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const LogFileMode &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const LogFileMode &other, value, other._value);

public: // accessors
    /// Get the raw file mode value.
    /// @return The embedded value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this mode to its canonical configuration identifier.
    /// @return `overwrite` or `append`.
    [[nodiscard]] auto toString() const -> text::String;
    /// Get all canonical configuration identifiers.
    /// @return The identifiers accepted by `fromString()`, irrespective of ASCII letter case.
    [[nodiscard]] static auto allStrings() -> text::StringList;
    /// Parse a canonical file mode identifier case-insensitively using ASCII folding.
    /// @param text The identifier to parse.
    /// @return The parsed mode, or an empty optional if the identifier is unknown.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<LogFileMode>;
    /// Parse a canonical file mode identifier.
    /// @param text The identifier to parse.
    /// @return The parsed mode.
    /// @throws err::ParseError If the identifier is unknown.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> LogFileMode;

private:
    Value _value{Append}; ///< Raw file mode value.
};

}
