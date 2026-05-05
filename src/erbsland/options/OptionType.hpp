// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"

#include <cstdint>

namespace erbsland::options {

/// The value type accepted by an option.
/// @tested{OptionsFrameworkTest}
class OptionType {
public:
    /// The supported option types.
    enum Type : uint8_t {
        Flag,    ///< A boolean flag or flag count.
        Integer, ///< A single integer or integer list.
        Text,    ///< A single text value or text list.
        Choice,  ///< A text value bound to a list of choices.
    };

public:
    /// Create a flag option type.
    constexpr OptionType() noexcept = default;
    /// Create an option type from a raw type value.
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
    [[nodiscard]] constexpr auto type() const noexcept -> Type { return _type; }
    /// Set the raw option type.
    void setType(Type type) noexcept { _type = type; }

public: // conversion
    /// Convert the option type to a text representation.
    [[nodiscard]] auto toString() const -> text::StringView;

private:
    Type _type{Flag}; ///< The raw option type.
};

}
