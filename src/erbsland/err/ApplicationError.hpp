// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Exception.hpp"

#include "../text/StringView.hpp"
#include "../unit/ExitCode.hpp"

namespace erbsland::err {

/// A generic application error that is meant to terminate a running application with the given message and exit code.
class ApplicationError : public Exception {
public:
    /// Create an application error with the given reason and exit-code.
    /// @param reason The reason for the error.
    /// @param exitCode The exit code to use when terminating the application.
    explicit ApplicationError(
        text::StringView reason, const unit::ExitCode exitCode = unit::ExitCode::failure()) noexcept :
        Exception{std::move(reason)}, _exitCode{exitCode} {}

public: // overrides
    [[nodiscard]] auto toString() const noexcept -> text::StringView override;

public: // accessors
    /// Access the exit code
    [[nodiscard]] auto exitCode() const noexcept -> unit::ExitCode { return _exitCode; }

private:
    unit::ExitCode _exitCode;
};

}
