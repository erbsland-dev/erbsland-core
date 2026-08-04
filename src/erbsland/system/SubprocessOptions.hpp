// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SubprocessOptions_fwd.hpp"
#include "SubprocessOutputMode.hpp"

#include "../path/Path.hpp"
#include "../text/String.hpp"
#include "../text/StringMap.hpp"
#include "../unit/ByteLength.hpp"

#include <optional>
#include <utility>

namespace erbsland::system {

/// Configures the launch environment and standard streams of a subprocess.
/// Environment changes are applied after optional inheritance. Assigning no value removes the variable from the child
/// environment. Names retain the native platform's case-sensitivity rules.
/// @seedoc{/reference/system/subprocess}
/// @tested{SubprocessInteropTest}
class SubprocessOptions final {
public:
    /// Default number of bytes retained for each captured output stream.
    static constexpr auto cDefaultCaptureLimit = unit::ByteLength{1024U * 1024U};
    /// Maximum configurable number of retained bytes for each captured output stream.
    static constexpr auto cMaximumCaptureLimit = unit::ByteLength{64U * 1024U * 1024U};

public: // accessors
    /// Get the optional child working directory.
    [[nodiscard]] auto workingDirectory() const noexcept -> const std::optional<path::Path> & {
        return _workingDirectory;
    }
    /// Set the child working directory.
    auto setWorkingDirectory(path::Path value) noexcept -> SubprocessOptions & {
        _workingDirectory = std::move(value);
        return *this;
    }
    /// Clear the configured child working directory.
    auto clearWorkingDirectory() noexcept -> SubprocessOptions & {
        _workingDirectory.reset();
        return *this;
    }
    /// Test if the child inherits the current process environment.
    [[nodiscard]] auto inheritsEnvironment() const noexcept -> bool { return _inheritEnvironment; }
    /// Select whether the child inherits the current process environment.
    auto setInheritEnvironment(const bool value) noexcept -> SubprocessOptions & {
        _inheritEnvironment = value;
        return *this;
    }
    /// Get environment assignments and removals applied to the child environment.
    [[nodiscard]] auto environmentChanges() const noexcept -> const text::StringMap<std::optional<text::String>> & {
        return _environmentChanges;
    }
    /// Assign one child environment variable.
    auto setEnvironmentVariable(const text::String &name, text::String value) -> SubprocessOptions & {
        _environmentChanges.set(name, std::move(value));
        return *this;
    }
    /// Remove one variable from the child environment.
    auto removeEnvironmentVariable(const text::String &name) -> SubprocessOptions & {
        _environmentChanges.set(name, std::nullopt);
        return *this;
    }
    /// Remove all pending child environment changes.
    auto clearEnvironmentChanges() noexcept -> SubprocessOptions & {
        _environmentChanges.clear();
        return *this;
    }
    /// Test if the child inherits the parent's standard input stream.
    [[nodiscard]] auto inheritsStandardInput() const noexcept -> bool { return _inheritStandardInput; }
    /// Select whether the child inherits standard input or receives an immediately closed stream.
    auto setInheritStandardInput(const bool value) noexcept -> SubprocessOptions & {
        _inheritStandardInput = value;
        return *this;
    }
    /// Get the standard output handling mode.
    [[nodiscard]] auto standardOutputMode() const noexcept -> SubprocessOutputMode { return _standardOutputMode; }
    /// Set the standard output handling mode.
    auto setStandardOutputMode(const SubprocessOutputMode value) noexcept -> SubprocessOptions & {
        _standardOutputMode = value;
        return *this;
    }
    /// Get the standard error handling mode.
    [[nodiscard]] auto standardErrorMode() const noexcept -> SubprocessOutputMode { return _standardErrorMode; }
    /// Set the standard error handling mode.
    auto setStandardErrorMode(const SubprocessOutputMode value) noexcept -> SubprocessOptions & {
        _standardErrorMode = value;
        return *this;
    }
    /// Test if standard error is redirected into standard output.
    [[nodiscard]] auto mergesStandardError() const noexcept -> bool { return _mergeStandardError; }
    /// Select whether standard error is redirected into standard output.
    auto setMergeStandardError(const bool value) noexcept -> SubprocessOptions & {
        _mergeStandardError = value;
        return *this;
    }
    /// Get the number of bytes retained for each captured output stream.
    [[nodiscard]] auto captureLimit() const noexcept -> unit::ByteLength { return _captureLimit; }
    /// Set the number of bytes retained for each captured output stream.
    auto setCaptureLimit(const unit::ByteLength value) noexcept -> SubprocessOptions & {
        _captureLimit = value;
        return *this;
    }

private:
    std::optional<path::Path> _workingDirectory;                      ///< Optional child working directory.
    bool _inheritEnvironment{true};                                   ///< Whether to inherit the parent environment.
    text::StringMap<std::optional<text::String>> _environmentChanges; ///< Child environment assignments/removals.
    bool _inheritStandardInput{true};                                 ///< Whether to inherit standard input.
    SubprocessOutputMode _standardOutputMode{SubprocessOutputMode::Inherit}; ///< Standard output policy.
    SubprocessOutputMode _standardErrorMode{SubprocessOutputMode::Inherit};  ///< Standard error policy.
    bool _mergeStandardError{};                           ///< Whether standard error is redirected to output.
    unit::ByteLength _captureLimit{cDefaultCaptureLimit}; ///< Retained bytes for each captured stream.
};

}
