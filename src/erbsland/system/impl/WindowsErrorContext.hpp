// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../PlatformErrorContext.hpp"

#include "../../core/impl/WindowsApi.hpp"
#include "../../text/StringEditor.hpp"

namespace erbsland::system::impl {

/// Immutable diagnostic details captured from a Windows system error.
/// @tested{DiagnosticTest}
class WindowsErrorContext final : public PlatformErrorContext {
public:
    /// Native Windows error-code type.
    using ErrorCode = DWORD;

public:
    /// Create a context from an already captured code and message.
    explicit WindowsErrorContext(ErrorCode errorCode, text::String errorMessage = {}) noexcept;

public:
    /// Capture an explicit Windows error code and its current system message.
    [[nodiscard]] static auto fromErrorCode(ErrorCode errorCode) -> std::shared_ptr<const WindowsErrorContext>;
    /// Capture `GetLastError()` and its current system message.
    [[nodiscard]] static auto fromLastError() -> std::shared_ptr<const WindowsErrorContext>;

public: // implement PlatformErrorContext
    [[nodiscard]] auto category() const noexcept -> PlatformErrorCategory override;
    [[nodiscard]] auto toString() const noexcept -> text::String override;
    [[nodiscard]] auto toTextDocument() const -> text::TextDocument override;

public: // accessors
    /// Get the captured Windows error code.
    [[nodiscard]] auto errorCode() const noexcept -> ErrorCode { return _errorCode; }
    /// Get the captured Windows error message.
    [[nodiscard]] auto errorMessage() const noexcept -> text::String { return _errorMessage; }

private:
    /// Convert a Windows error code into its system-provided message text.
    [[nodiscard]] static auto messageFromErrorCode(ErrorCode errorCode) -> text::String;

private:
    ErrorCode _errorCode{};
    text::String _errorMessage;
};

}
