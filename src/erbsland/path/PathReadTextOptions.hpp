// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../stream/InputStreamSettings.hpp"
#include "../text/EncodingErrorMode.hpp"
#include "../text/StringBomMode.hpp"
#include "../text/StringEncoding.hpp"
#include "../time/TimeDelta.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/CpLength.hpp"

namespace erbsland::path {

/// The options for reading text from a file.
/// @tested{PathContentTest StreamSettingsTest}
class PathReadTextOptions final {
public:
    /// The default timeout for whole-file path operations.
    inline static const auto cDefaultTimeout = time::TimeDelta::seconds(60);

public:
    /// Create the default options.
    PathReadTextOptions() = default;

    /// Create UTF-8, replacement char and automatic BOM with a maximum byte length.
    /// @param maximumByteLength The maximum byte length to read.
    PathReadTextOptions(unit::ByteLength maximumByteLength); // NOLINT(*-explicit-constructor)
    /// Create UTF-8, replacement char and automatic BOM with a maximum code-point length.
    /// @param maximumCpLength The maximum code-point length to read.
    PathReadTextOptions(unit::CpLength maximumCpLength); // NOLINT(*-explicit-constructor)
    /// Use defaults with a different encoding
    /// @param encoding The encoding to use.
    PathReadTextOptions(text::StringEncoding encoding); // NOLINT(*-explicit-constructor)

    // defaults
    ~PathReadTextOptions() = default;
    PathReadTextOptions(const PathReadTextOptions &) = default;
    auto operator=(const PathReadTextOptions &) -> PathReadTextOptions & = default;

public:
    /// Get the text encoding.
    [[nodiscard]] auto encoding() const -> text::StringEncoding { return _encoding; }
    /// Set the text encoding.
    auto setEncoding(text::StringEncoding value) -> PathReadTextOptions &;
    /// Get the byte-order-mark mode.
    [[nodiscard]] auto bomMode() const -> text::StringBomMode { return _bomMode; }
    /// Set the byte-order-mark mode.
    auto setBomMode(text::StringBomMode value) -> PathReadTextOptions &;
    /// Get the handling mode for encoding errors.
    [[nodiscard]] auto encodingErrorMode() const -> text::EncodingErrorMode { return _encodingErrorMode; }
    /// Set the handling mode for encoding errors.
    auto setEncodingErrorMode(text::EncodingErrorMode value) -> PathReadTextOptions &;
    /// Get the maximum number of bytes to read.
    [[nodiscard]] auto maximumByteLength() const -> unit::ByteLength { return _maximumByteLength; }
    /// Set the maximum number of bytes to read.
    auto setMaximumByteLength(unit::ByteLength value) -> PathReadTextOptions &;
    /// Get the maximum number of code points to read.
    [[nodiscard]] auto maximumCpLength() const -> unit::CpLength { return _maximumCpLength; }
    /// Set the maximum number of code points to read.
    auto setMaximumCpLength(unit::CpLength value) -> PathReadTextOptions &;
    /// Get the maximum wait for one stream operation.
    [[nodiscard]] auto timeout() const noexcept -> time::TimeDelta { return _streamSettings.timeout(); }
    /// Set the maximum wait for one stream operation.
    auto setTimeout(const time::TimeDelta value) noexcept -> PathReadTextOptions & {
        _streamSettings.setTimeout(value);
        return *this;
    }
    /// Get the stream settings.
    [[nodiscard]] auto streamSettings() const noexcept -> const stream::InputStreamSettings & {
        return _streamSettings;
    }
    /// Set the stream settings.
    auto setStreamSettings(const stream::InputStreamSettings &value) noexcept -> PathReadTextOptions & {
        _streamSettings = value;
        return *this;
    }

private:
    text::StringEncoding _encoding = text::StringEncoding::Utf8;
    text::StringBomMode _bomMode = text::StringBomMode::Automatic;
    text::EncodingErrorMode _encodingErrorMode = text::EncodingErrorMode::Replace;
    unit::ByteLength _maximumByteLength = unit::ByteLength{10'000'000LL};
    unit::CpLength _maximumCpLength = unit::CpLength::infinite();
    stream::InputStreamSettings _streamSettings{stream::InputStreamSettings{}.setTimeout(cDefaultTimeout)};
};

}
