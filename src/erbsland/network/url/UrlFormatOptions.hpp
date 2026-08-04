// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HostNameFormat.hpp"

namespace erbsland::network {

/// Options for formatting a URL.
/// @tested{UrlTest}
class UrlFormatOptions final {
public: // accessors
    /// Get the host-name format.
    [[nodiscard]] constexpr auto hostNameFormat() const noexcept -> HostNameFormat { return _hostNameFormat; }
    /// Set the host-name format.
    constexpr auto setHostNameFormat(const HostNameFormat value) noexcept -> UrlFormatOptions & {
        _hostNameFormat = value;
        return *this;
    }
    /// Test whether default ports are included.
    [[nodiscard]] constexpr auto includesDefaultPort() const noexcept -> bool { return _includeDefaultPort; }
    /// Include or omit default ports.
    constexpr auto setIncludeDefaultPort(const bool value) noexcept -> UrlFormatOptions & {
        _includeDefaultPort = value;
        return *this;
    }
    /// Test whether the fragment is included.
    [[nodiscard]] constexpr auto includesFragment() const noexcept -> bool { return _includeFragment; }
    /// Include or omit the fragment.
    constexpr auto setIncludeFragment(const bool value) noexcept -> UrlFormatOptions & {
        _includeFragment = value;
        return *this;
    }
    /// Test whether passwords are redacted.
    [[nodiscard]] constexpr auto redactsPassword() const noexcept -> bool { return _redactPassword; }
    /// Enable or disable password redaction.
    constexpr auto setRedactPassword(const bool value) noexcept -> UrlFormatOptions & {
        _redactPassword = value;
        return *this;
    }

private:
    HostNameFormat _hostNameFormat{HostNameFormat::IdnaAscii}; ///< Host-name output form.
    bool _includeDefaultPort{false};                           ///< Emit default ports explicitly.
    bool _includeFragment{true};                               ///< Emit the fragment.
    bool _redactPassword{true};                                ///< Replace passwords with a fixed marker.
};

}
