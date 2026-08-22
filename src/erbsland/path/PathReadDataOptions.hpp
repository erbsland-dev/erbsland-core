// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SymlinkMode.hpp"

#include "../stream/InputStreamSettings.hpp"
#include "../time/TimeDelta.hpp"
#include "../unit/ByteLength.hpp"

namespace erbsland::path {

/// The options for reading data from a file.
/// @tested{PathContentTest StreamSettingsTest}
class PathReadDataOptions final {
public:
    /// The default timeout for whole-file path operations.
    inline static const auto cDefaultTimeout = time::TimeDelta::seconds(60);
    /// The default maximum byte length for whole-file read operations.
    inline static const auto cDefaultMaximumByteLength = unit::ByteLength{10'000'000LL};

public:
    /// Create path-read options with their default values.
    PathReadDataOptions() noexcept = default;

public: // stream settings
    /// Get the maximum number of bytes to read.
    [[nodiscard]] auto maximumByteLength() const noexcept -> unit::ByteLength { return _maximumByteLength; }
    /// Set the maximum number of bytes to read.
    auto setMaximumByteLength(const unit::ByteLength value) noexcept -> PathReadDataOptions & {
        _maximumByteLength = value;
        return *this;
    }
    /// Get how symbolic links are handled while opening the input file.
    [[nodiscard]] auto symlinkMode() const noexcept -> SymlinkMode { return _symlinkMode; }
    /// Set how symbolic links are handled while opening the input file.
    /// `Use` has the same restrictive behavior as `Skip`, because a symbolic link has no regular file content.
    auto setSymlinkMode(const SymlinkMode value) noexcept -> PathReadDataOptions & {
        _symlinkMode = value;
        return *this;
    }
    /// Get the maximum wait for one stream operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _streamSettings.timeout(); }
    /// Set the maximum wait for one stream operation.
    auto setTimeout(const time::TimeDelta value) noexcept -> PathReadDataOptions & {
        _streamSettings.setTimeout(value);
        return *this;
    }
    /// Get the stream settings.
    [[nodiscard]] auto streamSettings() const noexcept -> const stream::InputStreamSettings & {
        return _streamSettings;
    }
    /// Set the stream settings.
    auto setStreamSettings(const stream::InputStreamSettings &value) noexcept -> PathReadDataOptions & {
        _streamSettings = value;
        return *this;
    }

private:
    unit::ByteLength _maximumByteLength = cDefaultMaximumByteLength;
    SymlinkMode _symlinkMode{SymlinkMode::Follow};
    stream::InputStreamSettings _streamSettings{stream::InputStreamSettings{}.setTimeout(cDefaultTimeout)};
};

}
