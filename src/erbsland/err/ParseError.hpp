// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "RuntimeError.hpp"

#include "../unit/CpIndex.hpp"

#include <string_view>
#include <utility>

namespace erbsland::err {

/// A parse error exception.
/// These exceptions are thrown when text cannot be parsed as the requested value.
/// @tested{IntegerConversionTest}
class ParseError : public RuntimeError {
public:
    /// Create a parse error with a reason.
    /// @param reason The reason for the parse error.
    /// @param position The optional code-point position of the parse error.
    explicit ParseError(text::StringView reason, unit::CpIndex position = unit::CpIndex::noIndex()) noexcept;
    /// @overload
    explicit ParseError(std::string_view reason, unit::CpIndex position = unit::CpIndex::noIndex()) noexcept;

    // defaults
    ~ParseError() override = default;

public: // implement Exception
    [[nodiscard]] auto toString() const noexcept -> text::StringView override;
    [[nodiscard]] auto diagnostic() const -> DiagnosticConstPtr override;

public: // accessors
    /// Test if this error has an associated code-point position.
    [[nodiscard]] auto hasPosition() const noexcept -> bool;
    /// Get the code-point position of the parse error, or no-index if no position is available.
    [[nodiscard]] auto position() const noexcept -> unit::CpIndex;

private:
    unit::CpIndex _position{unit::CpIndex::noIndex()}; ///< The optional code-point error position.
};

}
