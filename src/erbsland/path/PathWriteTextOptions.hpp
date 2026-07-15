// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PathCreateFileOptions.hpp"

#include "../stream/OutputStreamSettings.hpp"
#include "../text/EncodingErrorMode.hpp"
#include "../text/StringBomMode.hpp"
#include "../text/StringEncoding.hpp"
#include "../time/TimeDelta.hpp"

namespace erbsland::path {

/// The options for writing text to a file.
/// @tested{PathContentTest StreamSettingsTest}
class PathWriteTextOptions final {
public:
    /// The default timeout for whole-file path operations.
    inline static const auto cDefaultTimeout = time::TimeDelta::seconds(60);

public:
    /// Create the default options.
    PathWriteTextOptions() = default;

    /// Use defaults with a different encoding
    /// @param encoding The encoding to use.
    PathWriteTextOptions(text::StringEncoding encoding); // NOLINT(*-explicit-constructor)

    // defaults
    ~PathWriteTextOptions() = default;
    PathWriteTextOptions(const PathWriteTextOptions &) = default;
    auto operator=(const PathWriteTextOptions &) -> PathWriteTextOptions & = default;

public: // file create options
    /// Create the parent directories if they do not exist.
    [[nodiscard]] auto createParents() const noexcept -> bool { return _createFileOptions.createParents(); }
    /// Set whether to create the parent directories if they do not exist.
    auto setCreateParents(const bool value) noexcept -> PathWriteTextOptions & {
        _createFileOptions.setCreateParents(value);
        return *this;
    }
    /// The creation mode for the file.
    [[nodiscard]] auto creationMode() const noexcept -> PathCreateMode { return _createFileOptions.creationMode(); }
    /// Set the creation mode for the file.
    auto setCreationMode(const PathCreateMode mode) noexcept -> PathWriteTextOptions & {
        _createFileOptions.setCreationMode(mode);
        return *this;
    }
    /// The access profile for newly created files.
    [[nodiscard]] auto accessProfile() const noexcept -> PathAccessProfile {
        return _createFileOptions.accessProfile();
    }
    /// Set the access profile for newly created files.
    auto setAccessProfile(const PathAccessProfile value) noexcept -> PathWriteTextOptions & {
        _createFileOptions.setAccessProfile(value);
        return *this;
    }

public:
    /// Get the text encoding.
    [[nodiscard]] auto encoding() const noexcept -> text::StringEncoding { return _encoding; }
    /// Set the text encoding.
    auto setEncoding(const text::StringEncoding value) noexcept -> PathWriteTextOptions & {
        _encoding = value;
        return *this;
    }
    /// Get the byte-order-mark mode.
    [[nodiscard]] auto bomMode() const noexcept -> text::StringBomMode { return _bomMode; }
    /// Set the byte-order-mark mode.
    auto setBomMode(const text::StringBomMode value) noexcept -> PathWriteTextOptions & {
        _bomMode = value;
        return *this;
    }
    /// Get the handling mode for encoding errors.
    [[nodiscard]] auto encodingErrorMode() const noexcept -> text::EncodingErrorMode { return _encodingErrorMode; }
    /// Set the handling mode for encoding errors.
    auto setEncodingErrorMode(const text::EncodingErrorMode value) noexcept -> PathWriteTextOptions & {
        _encodingErrorMode = value;
        return *this;
    }
    /// Get the maximum wait for one stream operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _streamSettings.timeout(); }
    /// Set the maximum wait for one stream operation.
    auto setTimeout(const time::TimeDelta value) noexcept -> PathWriteTextOptions & {
        _streamSettings.setTimeout(value);
        return *this;
    }
    /// Get the stream settings.
    [[nodiscard]] auto streamSettings() const noexcept -> const stream::OutputStreamSettings & {
        return _streamSettings;
    }
    /// Set the stream settings.
    auto setStreamSettings(const stream::OutputStreamSettings &value) noexcept -> PathWriteTextOptions & {
        _streamSettings = value;
        return *this;
    }

private:
    PathCreateFileOptions _createFileOptions;
    text::StringEncoding _encoding = text::StringEncoding::Utf8;
    text::StringBomMode _bomMode = text::StringBomMode::Automatic;
    text::EncodingErrorMode _encodingErrorMode = text::EncodingErrorMode::Replace;
    stream::OutputStreamSettings _streamSettings{stream::OutputStreamSettings{}.setTimeout(cDefaultTimeout)};
};

}
