// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/StringEditor.hpp"

#include <cstdint>

namespace erbsland::options {

/// The value type accepted by an option.
///
/// The parser stores values according to this type and the option maximum. Flags count occurrences, integer and text
/// options store one value or a list, and choice options store the configured choice text that matched the argument.
/// @tested{OptionsFrameworkTest}
class OptionType {
public:
    /// The supported option types.
    enum Type : uint8_t {
        Flag,          ///< A boolean switch; bare use is true and compatible parsers accept explicit boolean values.
        Integer,       ///< A signed decimal integer argument.
        Text,          ///< An arbitrary text argument.
        SensitiveText, ///< A single sensitive text argument stored in protected memory.
        Choice,        ///< A text argument that must match one configured `OptionChoice`.
    };

public:
    /// Create a flag option type.
    constexpr OptionType() noexcept = default;
    /// Create an option type from a raw type value.
    /// @param type The raw type value to store.
    constexpr OptionType(const Type type) noexcept : _type{type} {} // NOLINT(*-explicit-constructor)

    // defaults
    ~OptionType() = default;
    OptionType(const OptionType &) noexcept = default;
    auto operator=(const OptionType &) noexcept -> OptionType & = default;

public: // operators
    /// Compare two option types.
    [[nodiscard]] constexpr auto operator==(const OptionType &other) const noexcept -> bool = default;
    /// Compare this option type with a raw type.
    [[nodiscard]] constexpr auto operator==(Type type) const noexcept -> bool { return _type == type; }

public: // accessors
    /// Get the raw option type.
    /// @return The raw type value.
    [[nodiscard]] constexpr auto type() const noexcept -> Type { return _type; }
    /// Set the raw option type.
    /// @param type The new type value.
    void setType(Type type) noexcept { _type = type; }

public: // conversion
    /// Convert the option type to a text representation.
    /// @return A stable lowercase name for diagnostics and generated help.
    [[nodiscard]] auto toString() const -> text::String;

private:
    Type _type{Flag}; ///< The raw option type.
};

}
