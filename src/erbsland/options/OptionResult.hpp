// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionErrorContext.hpp"
#include "OptionResult_fwd.hpp"
#include "OptionResultStatus.hpp"
#include "OptionSensitiveTextLocation.hpp"
#include "OptionValues_fwd.hpp"

namespace erbsland::options {

namespace impl {
class OptionParser;
}

/// The result of processing command line arguments.
///
/// `parse()` returns this object for every outcome, including successful parsing, help/version requests, and errors.
/// @tested{OptionsFrameworkTest OptionsParserTest}
class OptionResult {
    friend class impl::OptionParser;

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
    /// @param values Parsed values associated with this result.
    void setValues(OptionValuesPtr values) noexcept { _values = std::move(values); }
    /// Get the result status.
    [[nodiscard]] auto status() const noexcept -> OptionResultStatus { return _status; }
    /// Set the result status.
    /// @param status New parser status.
    void setStatus(const OptionResultStatus status) noexcept { _status = status; }
    /// Access the error context.
    /// @return Structured error context when `status()` is `OptionResultStatus::Error`.
    [[nodiscard]] auto errorContext() const noexcept -> const std::optional<OptionErrorContext> & {
        return _errorContext;
    }
    /// Set the error context.
    /// @param errorContext Error context for a failed parse, or empty for non-error results.
    void setErrorContext(std::optional<OptionErrorContext> errorContext) noexcept {
        _errorContext = std::move(errorContext);
    }
    /// Get the command-line locations that contained sensitive text.
    /// Locations are available for successful, display-request, and error results.
    /// @return The sensitive suffix locations in parsing order.
    [[nodiscard]] auto sensitiveTextLocations() const noexcept -> const OptionSensitiveTextLocations & {
        return _sensitiveTextLocations;
    }

private:
    void setSensitiveTextLocations(OptionSensitiveTextLocations locations) {
        _sensitiveTextLocations = std::move(locations);
    }

private:
    OptionValuesPtr _values;                                 ///< The parsed option values.
    OptionResultStatus _status{OptionResultStatus::Success}; ///< The result status.
    std::optional<OptionErrorContext> _errorContext;         ///< The error context.
    OptionSensitiveTextLocations _sensitiveTextLocations;    ///< Sensitive source locations found while parsing.
};

}
