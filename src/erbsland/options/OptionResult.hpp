// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionErrorContext.hpp"
#include "OptionResultStatus.hpp"
#include "OptionValues_fwd.hpp"

namespace erbsland::options {

/// The result of processing command line arguments.
/// @tested{OptionsFrameworkTest}
class OptionResult {
public:
    OptionResult() = default;

    // defaults
    ~OptionResult() = default;
    OptionResult(const OptionResult &) = default;
    auto operator=(const OptionResult &) -> OptionResult & = default;
    OptionResult(OptionResult &&) = default;
    auto operator=(OptionResult &&) -> OptionResult & = default;

public: // accessors
    /// Get the parsed values.
    [[nodiscard]] auto values() const noexcept -> const OptionValuesPtr & { return _values; }
    /// Set the parsed values.
    void setValues(OptionValuesPtr values) noexcept { _values = std::move(values); }
    /// Get the result status.
    [[nodiscard]] auto status() const noexcept -> OptionResultStatus { return _status; }
    /// Set the result status.
    void setStatus(const OptionResultStatus status) noexcept { _status = status; }
    /// Access the error context
    [[nodiscard]] auto errorContext() const noexcept -> const std::optional<OptionErrorContext> & {
        return _errorContext;
    }
    /// Set the error context.
    void setErrorContext(std::optional<OptionErrorContext> errorContext) noexcept {
        _errorContext = std::move(errorContext);
    }

private:
    OptionValuesPtr _values;                                 ///< The parsed option values.
    OptionResultStatus _status{OptionResultStatus::Success}; ///< The result status.
    std::optional<OptionErrorContext> _errorContext;         ///< The error context.
};

}
