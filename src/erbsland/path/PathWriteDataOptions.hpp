// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathCreateFileOptions.hpp"

#include "../stream/OutputStreamSettings.hpp"
#include "../time/TimeDelta.hpp"

namespace erbsland::path {

/// The options for writing data to a file.
/// @tested{PathContentTest StreamSettingsTest}
class PathWriteDataOptions final {
public:
    /// The default timeout for whole-file path operations.
    inline static const auto cDefaultTimeout = time::TimeDelta::seconds(60);

public:
    /// Create the default options.
    PathWriteDataOptions() = default;

    // defaults
    ~PathWriteDataOptions() = default;
    PathWriteDataOptions(const PathWriteDataOptions &) = default;
    auto operator=(const PathWriteDataOptions &) -> PathWriteDataOptions & = default;

public: // file create options
    /// Create the parent directories if they do not exist.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _createFileOptions.createParents(); }
    /// Set whether to create the parent directories if they do not exist.
    auto setCreateParents(const bool value) noexcept -> PathWriteDataOptions & {
        _createFileOptions.setCreateParents(value);
        return *this;
    }
    /// The creation mode for the file.
    [[nodiscard]] auto creationMode() const noexcept -> PathCreateMode { return _createFileOptions.creationMode(); }
    /// Set the creation mode for the file.
    auto setCreationMode(const PathCreateMode mode) noexcept -> PathWriteDataOptions & {
        _createFileOptions.setCreationMode(mode);
        return *this;
    }
    /// The access profile for newly created files.
    [[nodiscard]] auto accessProfile() const noexcept -> PathAccessProfile {
        return _createFileOptions.accessProfile();
    }
    /// Set the access profile for newly created files.
    auto setAccessProfile(const PathAccessProfile value) noexcept -> PathWriteDataOptions & {
        _createFileOptions.setAccessProfile(value);
        return *this;
    }

public: // stream settings
    /// Get the maximum wait for one stream operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _streamSettings.timeout(); }
    /// Set the maximum wait for one stream operation.
    auto setTimeout(const time::TimeDelta value) noexcept -> PathWriteDataOptions & {
        _streamSettings.setTimeout(value);
        return *this;
    }
    /// Get the stream settings.
    [[nodiscard]] auto streamSettings() const noexcept -> const stream::OutputStreamSettings & {
        return _streamSettings;
    }
    /// Set the stream settings.
    auto setStreamSettings(const stream::OutputStreamSettings &value) noexcept -> PathWriteDataOptions & {
        _streamSettings = value;
        return *this;
    }

private:
    PathCreateFileOptions _createFileOptions;
    stream::OutputStreamSettings _streamSettings{stream::OutputStreamSettings{}.setTimeout(cDefaultTimeout)};
};

}
