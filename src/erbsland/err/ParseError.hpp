// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RuntimeError.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/CodeLocation.hpp"
#include "../unit/CpIndex.hpp"

#include <string_view>
#include <utility>
#include <variant>

namespace erbsland::err {

/// A parse error exception.
/// These exceptions are thrown when text cannot be parsed as the requested value.
/// @tested{IntegerConversionTest}
class ParseError : public RuntimeError {
public:
    /// A position in a parsed document.
    using Position = std::variant<std::monostate, unit::CpIndex, unit::ByteIndex, unit::CodeLocation>;

public:
    /// Create a parse error with a reason.
    /// @param reason The reason for the parse error.
    /// @param position The optional code-point position of the parse error.
    explicit ParseError(text::String reason, const Position &position = {}) noexcept;
    /// @overload
    explicit ParseError(std::string_view reason, const Position &position = {}) noexcept;

    // defaults
    ~ParseError() override = default;

public: // implement Exception
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto diagnostic() const -> DiagnosticConstPtr override;

public: // accessors
    /// Test if this error has an associated position.
    [[nodiscard]] auto hasPosition() const noexcept -> bool;
    /// Get the position of the parse error.
    [[nodiscard]] auto position() const noexcept -> const Position &;
    /// Get a code-point position or no-index if there is no code-point index.
    [[nodiscard]] auto codePointIndex() const noexcept -> unit::CpIndex {
        const auto result = std::get_if<unit::CpIndex>(&_position);
        return result != nullptr ? *result : unit::CpIndex{};
    }
    /// Get a byte index or no-index if there is no byte index.
    [[nodiscard]] auto byteIndex() const noexcept -> unit::ByteIndex {
        const auto result = std::get_if<unit::ByteIndex>(&_position);
        return result != nullptr ? *result : unit::ByteIndex{};
    }
    /// Get a code location or no-location if there is no code location.
    [[nodiscard]] auto codeLocation() const noexcept -> unit::CodeLocation {
        const auto result = std::get_if<unit::CodeLocation>(&_position);
        return result != nullptr ? *result : unit::CodeLocation{};
    }

private:
    Position _position; ///< The optional code-point error position.
};

}
