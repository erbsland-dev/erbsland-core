// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/String.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <array>
#include <cstdint>

namespace erbsland::conf {

/// The category of an error.
/// @tested{ErrorCategoryTest ConfErrorTest}
class ConfErrorCategory {
public:
    /// The underlying enum type representing distinct categories of errors.
    enum Enum : uint8_t {
        // Error categories defined by the language specification
        IO = 1,            ///< A problem occurred while reading data from an I/O stream.
        Encoding = 2,      ///< The document contains a problem with UTF-8 encoding.
        UnexpectedEnd = 3, ///< The document ended unexpectedly.
        Character = 4,     ///< The document contains a control character that is not allowed.
        Syntax = 5,        ///< The document has a syntax error.
        LimitExceeded = 6, ///< The size of a name, text, or buffer exceeds the permitted limit.
        NameConflict = 7,  ///< The same name has already been defined earlier in the document.
        Indentation = 8,   ///< The indentation of a continued line does not match the previous line.
        Unsupported = 9,   ///< The requested feature/version is not supported by this parser.
        Signature = 10,    ///< The document’s signature was rejected.
        Access = 11,       ///< The document was rejected due to an access check.
        Validation = 12,   ///< The document did not meet one of the validation rules.
        Internal = 99,     ///< The parser encountered an unexpected internal error.
        // Additional categories for the API of this parser.
        ValueNotFound = 101, ///< A value with a given name-path couldn't be found.
        TypeMismatch = 102,  ///< A value exists but has the wrong type for a conversion.
    };

private:
    constexpr static auto _enumCount = 15;

public:
    /// Create an internal error category.
    constexpr ConfErrorCategory() = default;
    /// Create a new error category.
    /// @param value The error category enum.
    constexpr ConfErrorCategory(Enum value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    /// Default destructor.
    ~ConfErrorCategory() = default;
    /// Default copy constructor.
    ConfErrorCategory(const ConfErrorCategory &) = default;
    /// Default copy assignment.
    auto operator=(const ConfErrorCategory &) -> ConfErrorCategory & = default;

public: // operators
    /// Assign a new enum value to this error category.
    /// @param value The enum value to assign.
    /// @return Reference to this error category.
    auto operator=(Enum value) noexcept -> ConfErrorCategory & {
        _value = value;
        return *this;
    }
    /// Convert to the underlying enum value.
    /// @return The enum representation of this error category.
    constexpr explicit operator Enum() const noexcept { return _value; }
    /// Convert to the integer error code.
    /// @return The numeric code of this error category.
    explicit operator int() const noexcept { return toCode(); }

public: // comparison
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const ConfErrorCategory &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, Enum value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(Enum a, const ConfErrorCategory &b, a, b._value);

public: // conversion
    /// Get the text representation of this error category.
    /// @return A text representation of this error category.
    [[nodiscard]] auto toText() const noexcept -> text::String;
    /// Get the code for this error category.
    /// @return The error code.
    [[nodiscard]] auto toCode() const noexcept -> int;

private:
    Enum _value = Internal; ///< The internal value.
    using ValueEntry = std::tuple<Enum, int, text::String>;
    using ValueMap = std::array<ValueEntry, _enumCount>;
    static ValueMap _valueMap;
};

}
