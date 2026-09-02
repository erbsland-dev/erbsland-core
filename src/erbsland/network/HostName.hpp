// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostNameFormat.hpp"

#include "../text/String.hpp"

#include <compare>
#include <cstddef>
#include <functional>
#include <optional>

namespace erbsland::network {

/// A strict IDNA2008 host name with canonical Unicode and ASCII forms.
/// @seedoc{/reference/network/addressing_and_urls}
/// @tested{NetworkValueTest}
class HostName final {
public:
    // defaults/deletions
    HostName() = delete;

public: // operators
    /// Compare two host names.
    /// @param other The host name to compare with this host name.
    /// @return The strong lexical ordering between the names.
    [[nodiscard]] auto operator<=>(const HostName &other) const noexcept -> std::strong_ordering {
        return _name <=> other._name;
    }
    /// Test two canonical Unicode host names for equality.
    [[nodiscard]] auto operator==(const HostName &other) const noexcept -> bool { return _name == other._name; }

public: // accessors
    /// Get the host name.
    /// @return The retained host name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }

public: // conversion
    /// Get the host name as text.
    /// @param format The Unicode semantic or IDNA ASCII transport representation.
    /// @return The selected canonical representation.
    [[nodiscard]] auto toString(HostNameFormat format = HostNameFormat::Unicode) const -> text::String {
        return format == HostNameFormat::Unicode ? _name : _idnaAscii;
    }
    /// Calculate the host-name hash.
    /// @return The hash value.
    [[nodiscard]] auto toHash() const noexcept -> std::size_t { return _name.toHash(); }
    /// Validate and retain a platform-resolvable host name.
    /// @param text The complete host name without a port.
    /// @return The host name, or `std::nullopt` if the text is invalid.
    [[nodiscard]] static auto fromString(const text::String &text) noexcept -> std::optional<HostName>;
    /// Validate and retain a platform-resolvable host name.
    /// @param text The complete host name without a port.
    /// @return The validated host name.
    /// @throws err::ParseError If the text is not a valid host name.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &text) -> HostName;

private:
    /// Create a host name from already validated text.
    explicit HostName(text::String name, text::String idnaAscii) :
        _name{std::move(name)}, _idnaAscii{std::move(idnaAscii)} {}

private:
    text::String _name;      ///< The canonical lowercase NFC Unicode host name.
    text::String _idnaAscii; ///< The canonical lowercase ASCII transport form.
};

}

template <>
struct std::hash<erbsland::network::HostName> {
    auto operator()(const erbsland::network::HostName &value) const noexcept -> std::size_t { return value.toHash(); }
};
