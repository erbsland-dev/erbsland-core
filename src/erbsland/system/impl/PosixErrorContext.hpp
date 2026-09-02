// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../PlatformErrorContext.hpp"

#include "../../text/StringEditor.hpp"

namespace erbsland::system::impl {

/// Immutable diagnostic details captured from a POSIX `errno` value.
/// @tested{DiagnosticTest}
class PosixErrorContext final : public PlatformErrorContext {
public:
    /// Native POSIX error-code type.
    using ErrorCode = int;

public:
    /// Create a context from an already captured code and message.
    explicit PosixErrorContext(ErrorCode errorCode, text::String errorMessage = {}) noexcept;

public:
    /// Capture the current `errno` value and its current system message.
    [[nodiscard]] static auto fromErrno() -> std::shared_ptr<const PosixErrorContext>;
    /// Capture the given POSIX error code and its current system message.
    [[nodiscard]] static auto fromErrorCode(ErrorCode errorCode) -> std::shared_ptr<const PosixErrorContext>;

public: // implement PlatformErrorContext
    [[nodiscard]] auto category() const noexcept -> PlatformErrorCategory override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument() const -> text::TextDocument override;

public: // accessors
    /// Get the captured POSIX error code.
    [[nodiscard]] auto errorCode() const noexcept -> ErrorCode { return _errorCode; }
    /// Get the captured POSIX error message.
    [[nodiscard]] auto errorMessage() const noexcept -> text::String { return _errorMessage; }

private:
    ErrorCode _errorCode{};
    text::String _errorMessage;
};

}
