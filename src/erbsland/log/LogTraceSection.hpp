// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/ValidateTraceSection.hpp"

#include "../text/Char_fwd.hpp"
#include "../text/String.hpp"

#include <compare>
#include <variant>

namespace erbsland::log {

/// An optional trace section.
/// @seedoc{/reference/log/logging}
/// @tested{LogTraceSectionTest LogCoreTest}
class LogTraceSection final {
    /// The storage type holding an owned string or a static literal.
    using NameStorage = std::variant<text::StringLiteral, text::String>;
    /// An empty string literal for default construction.
    constexpr static auto cEmptyString = text::StringLiteral{""};

public:
    /// Create an empty trace section.
    constexpr LogTraceSection() : _value(cEmptyString) {}
    /// Validate and create a trace section.
    /// @param name An ASCII configuration identifier of at most 100 characters, starting with a letter and using
    /// letters, digits, `_`, or `-`; empty selects no section.
    constexpr explicit LogTraceSection(const text::StringLiteral &name) : _value(name) {
        impl::validateTraceSectionName(name);
    }
    /// Validate and create a trace section.
    /// @param name An ASCII configuration identifier of at most 100 characters, starting with a letter and using
    /// letters, digits, `_`, or `-`; empty selects no section.
    explicit LogTraceSection(const text::String &name);

    // defaults
    ~LogTraceSection() = default;
    LogTraceSection(const LogTraceSection &) = default;
    LogTraceSection(LogTraceSection &&) noexcept = default;
    auto operator=(const LogTraceSection &) -> LogTraceSection & = default;
    auto operator=(LogTraceSection &&) noexcept -> LogTraceSection & = default;

public: // operators
    /// Compare a trace section with another one for equality.
    auto operator==(const LogTraceSection &other) const noexcept -> bool;

public: // accessors and tests
    /// Test whether this trace section is unnamed.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool {
        return std::visit([](const auto &name) noexcept -> bool { return name.isEmpty(); }, _value);
    }
    /// Convert this section to its identifier string.
    [[nodiscard]] auto toString() const noexcept -> text::String {
        return std::visit([](const auto &name) noexcept -> text::String { return name; }, _value);
    }

public: // validation
    /// Validate a trace-section identifier.
    /// @param name The identifier text to validate.
    /// @return `true` if `name` is empty or a valid trace-section identifier.
    [[nodiscard]] static auto isValidName(const text::String &name) noexcept -> bool;

private:
    /// Compare characters after folding ASCII case and mapping hyphens to underscores.
    /// @param left The first validated identifier character.
    /// @param right The second validated identifier character.
    /// @return The ordering of the folded characters.
    [[nodiscard]] static auto compareCharacters(text::Char left, text::Char right) noexcept -> std::strong_ordering;

private:
    NameStorage _value; ///< Identifier with its original spelling.
};

}
