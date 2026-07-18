// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathAccessProfile.hpp"

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ElementCount.hpp"

namespace erbsland::path {

/// Options for creating temporary directories.
/// @tested{PathTemporaryTest}
class PathTempDirectoryOptions final {
public:
    /// Create the default options.
    PathTempDirectoryOptions();

public:
    /// The prefix to add in front of the random name part.
    [[nodiscard]] auto prefix() const noexcept -> const text::String & { return _prefix; }
    /// Set the prefix to add in front of the random name part.
    auto setPrefix(const text::String &value) -> PathTempDirectoryOptions & {
        _prefix = text::String{value};
        return *this;
    }
    /// The suffix to add after the random name part.
    [[nodiscard]] auto suffix() const noexcept -> const text::String & { return _suffix; }
    /// Set the suffix to add after the random name part.
    auto setSuffix(const text::String &value) -> PathTempDirectoryOptions & {
        _suffix = text::String{value};
        return *this;
    }
    /// The number of random characters to place between prefix and suffix.
    [[nodiscard]] auto randomLength() const noexcept -> unit::CpLength { return _randomLength; }
    /// Set the number of random characters to place between prefix and suffix.
    auto setRandomLength(const unit::CpLength value) noexcept -> PathTempDirectoryOptions & {
        _randomLength = value;
        return *this;
    }
    /// The maximum number of generated names to try before giving up.
    [[nodiscard]] auto maximumAttempts() const noexcept -> unit::ElementCount { return _maximumAttempts; }
    /// Set the maximum number of generated names to try before giving up.
    auto setMaximumAttempts(const unit::ElementCount value) noexcept -> PathTempDirectoryOptions & {
        _maximumAttempts = value;
        return *this;
    }
    /// Remove the directory recursively when the last temporary directory handle is destroyed.
    [[nodiscard]] auto removeOnDestroy() const noexcept -> bool { return _removeOnDestroy; }
    /// Set whether to remove the directory when the last temporary directory handle is destroyed.
    auto setRemoveOnDestroy(const bool value) noexcept -> PathTempDirectoryOptions & {
        _removeOnDestroy = value;
        return *this;
    }
    /// The access profile for newly created temporary directories.
    [[nodiscard]] auto accessProfile() const noexcept -> PathAccessProfile { return _accessProfile; }
    /// Set the access profile for newly created temporary directories.
    auto setAccessProfile(const PathAccessProfile value) noexcept -> PathTempDirectoryOptions & {
        _accessProfile = value;
        return *this;
    }

private:
    text::String _prefix;
    text::String _suffix;
    unit::CpLength _randomLength{24U};
    unit::ElementCount _maximumAttempts{128U};
    bool _removeOnDestroy{true};
    PathAccessProfile _accessProfile{PathAccessProfile::Default};
};

}
