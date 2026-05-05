// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Exception.hpp"

#include "../options/OptionErrorContext.hpp"

namespace erbsland::err {

/// An error raised while processing command line options.
/// @tested{OptionsFrameworkTest OptionsUsageTest}
class OptionError : public Exception {
public:
    /// Create an option error.
    OptionError() noexcept = default;
    /// Create an option error with reason details.
    /// @param context The error context.
    explicit OptionError(options::OptionErrorContext context);

    // defaults
    ~OptionError() override = default;
    OptionError(const OptionError &) = default;
    OptionError(OptionError &&) = default;
    auto operator=(const OptionError &) -> OptionError & = default;
    auto operator=(OptionError &&) -> OptionError & = default;

public: // accessors
    /// Get the error context.
    [[nodiscard]] auto context() const noexcept -> options::OptionErrorContext { return _context; }

private:
    options::OptionErrorContext _context; ///< The error context.
};

}
