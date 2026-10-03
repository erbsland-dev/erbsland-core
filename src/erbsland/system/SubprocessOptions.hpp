// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EnvironmentChanges.hpp"
#include "SubprocessOptions_fwd.hpp"
#include "SubprocessOutputMode.hpp"

#include "../path/Path.hpp"
#include "../unit/ByteCount.hpp"

#include <optional>
#include <utility>

namespace erbsland::system {

/// Configures the launch environment and standard streams of a subprocess.
/// @seedoc{/reference/system/system_services}
/// @tested{SubprocessOutputTest EnvironmentChangesTest SubprocessEventTest}
class SubprocessOptions final {
public:
    /// Default number of bytes retained for each captured output stream.
    static constexpr auto cDefaultCaptureLimit = unit::ByteCount{1024U * 1024U};
    /// Maximum configurable number of retained bytes for each captured output stream.
    static constexpr auto cMaximumCaptureLimit = unit::ByteCount{64U * 1024U * 1024U};

public: // accessors
    /// Get the maximum bytes in one output callback.
    [[nodiscard]] auto maximumOutputChunkSize() const noexcept -> unit::ByteCount;
    /// Set the maximum output block size (finite, at least 256 bytes).
    auto setMaximumOutputChunkSize(unit::ByteCount value) noexcept -> SubprocessOptions &;
    /// Get the optional child working directory.
    [[nodiscard]] auto workingDirectory() const noexcept -> const std::optional<path::Path> &;
    /// Set the child working directory.
    auto setWorkingDirectory(path::Path value) noexcept -> SubprocessOptions &;
    /// Clear the configured child working directory.
    auto clearWorkingDirectory() noexcept -> SubprocessOptions &;
    /// Get the declarative environment changes applied when the child starts.
    [[nodiscard]] auto environmentChanges() const noexcept -> const EnvironmentChanges &;
    /// Edit the declarative environment changes applied when the child starts.
    [[nodiscard]] auto environmentChanges() noexcept -> EnvironmentChanges &;
    /// Replace the child environment configuration.
    auto setEnvironmentChanges(EnvironmentChanges value) noexcept -> SubprocessOptions &;
    /// Test if the child inherits the parent's standard input stream.
    [[nodiscard]] auto inheritsStandardInput() const noexcept -> bool;
    /// Select whether the child inherits standard input or receives an immediately closed stream.
    auto setInheritStandardInput(const bool value) noexcept -> SubprocessOptions &;
    /// Get the standard output handling mode.
    [[nodiscard]] auto standardOutputMode() const noexcept -> SubprocessOutputMode;
    /// Set the standard output handling mode.
    auto setStandardOutputMode(const SubprocessOutputMode value) noexcept -> SubprocessOptions &;
    /// Get the standard error handling mode.
    [[nodiscard]] auto standardErrorMode() const noexcept -> SubprocessOutputMode;
    /// Set the standard error handling mode.
    auto setStandardErrorMode(const SubprocessOutputMode value) noexcept -> SubprocessOptions &;
    /// Test if standard error is redirected into standard output.
    [[nodiscard]] auto mergesStandardError() const noexcept -> bool;
    /// Select whether standard error is redirected into standard output.
    auto setMergeStandardError(const bool value) noexcept -> SubprocessOptions &;
    /// Test whether one initial UTF-8 BOM is removed from each output stream.
    [[nodiscard]] auto removesOutputBom() const noexcept -> bool { return _removeOutputBom; }
    /// Select BOM removal; disable it when consuming byte-exact machine output.
    auto setRemoveOutputBom(bool value) noexcept -> SubprocessOptions &;
    /// Get the per-stream capture-prefix limit, or pending-byte budget when an output handler is set.
    [[nodiscard]] auto captureLimit() const noexcept -> unit::ByteCount;
    /// Set the number of bytes retained for each captured output stream.
    auto setCaptureLimit(const unit::ByteCount value) noexcept -> SubprocessOptions &;

private:
    unit::ByteCount _maximumOutputChunkSize{4096U};                          ///< Maximum bytes per output callback.
    std::optional<path::Path> _workingDirectory;                             ///< Optional child working directory.
    EnvironmentChanges _environmentChanges;                                  ///< Declarative child environment changes.
    bool _inheritStandardInput{true};                                        ///< Whether to inherit standard input.
    SubprocessOutputMode _standardOutputMode{SubprocessOutputMode::Inherit}; ///< Standard output policy.
    SubprocessOutputMode _standardErrorMode{SubprocessOutputMode::Inherit};  ///< Standard error policy.
    bool _removeOutputBom{true};                         ///< Preserve the historical text-output behavior by default.
    bool _mergeStandardError{};                          ///< Whether standard error is redirected to output.
    unit::ByteCount _captureLimit{cDefaultCaptureLimit}; ///< Retained bytes for each captured stream.
};

}
