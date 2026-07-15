// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionErrorContext.hpp"

#include "../err/RuntimeError.hpp"

namespace erbsland::options {

/// An error raised while processing command line options.
/// @tested{OptionsFrameworkTest OptionsUsageTest}
class OptionError : public err::RuntimeError {
public:
    /// Create an option error.
    OptionError() noexcept = default;
    /// Create an option error with reason details.
    /// @param context The error context.
    explicit OptionError(OptionErrorContext context);

    // defaults
    ~OptionError() override = default;
    OptionError(const OptionError &) = default;
    OptionError(OptionError &&) = default;
    auto operator=(const OptionError &) -> OptionError & = default;
    auto operator=(OptionError &&) -> OptionError & = default;

public: // accessors
    [[nodiscard]] auto diagnostic() const -> err::DiagnosticConstPtr override;

public: // accessors
    /// Get the error context.
    [[nodiscard]] auto context() const noexcept -> const OptionErrorContext & { return _context; }

private:
    OptionErrorContext _context; ///< The error context.
};

}
