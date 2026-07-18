// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathAccessProfile.hpp"

#include "../text/String.hpp"
#include "../text/StringEditor.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/ElementCount.hpp"

namespace erbsland::path {

/// Options for creating temporary files.
/// @tested{PathTemporaryTest}
class PathTempFileOptions final {
public:
    /// Create the default options.
    PathTempFileOptions();

public:
    /// The prefix to add in front of the random name part.
    [[nodiscard]] auto prefix() const noexcept -> const text::String & { return _prefix; }
    /// Set the prefix to add in front of the random name part.
    auto setPrefix(const text::String &value) -> PathTempFileOptions & {
        _prefix = text::String{value};
        return *this;
    }
    /// The suffix to add after the random name part.
    [[nodiscard]] auto suffix() const noexcept -> const text::String & { return _suffix; }
    /// Set the suffix to add after the random name part.
    auto setSuffix(const text::String &value) -> PathTempFileOptions & {
        _suffix = text::String{value};
        return *this;
    }
    /// The number of random characters to place between prefix and suffix.
    [[nodiscard]] auto randomLength() const noexcept -> unit::CpLength { return _randomLength; }
    /// Set the number of random characters to place between prefix and suffix.
    auto setRandomLength(const unit::CpLength value) noexcept -> PathTempFileOptions & {
        _randomLength = value;
        return *this;
    }
    /// The maximum number of generated names to try before giving up.
    [[nodiscard]] auto maximumAttempts() const noexcept -> unit::ElementCount { return _maximumAttempts; }
    /// Set the maximum number of generated names to try before giving up.
    auto setMaximumAttempts(const unit::ElementCount value) noexcept -> PathTempFileOptions & {
        _maximumAttempts = value;
        return *this;
    }
    /// Remove the file when the temporary stream is closed or destroyed.
    [[nodiscard]] auto removeOnClose() const noexcept -> bool { return _removeOnClose; }
    /// Set whether to remove the file when the temporary stream is closed or destroyed.
    auto setRemoveOnClose(const bool value) noexcept -> PathTempFileOptions & {
        _removeOnClose = value;
        return *this;
    }
    /// The access profile for newly created temporary files.
    [[nodiscard]] auto accessProfile() const noexcept -> PathAccessProfile { return _accessProfile; }
    /// Set the access profile for newly created temporary files.
    auto setAccessProfile(const PathAccessProfile value) noexcept -> PathTempFileOptions & {
        _accessProfile = value;
        return *this;
    }

private:
    text::String _prefix;
    text::String _suffix;
    unit::CpLength _randomLength{24U};
    unit::ElementCount _maximumAttempts{128U};
    bool _removeOnClose{true};
    PathAccessProfile _accessProfile{PathAccessProfile::Default};
};

}
