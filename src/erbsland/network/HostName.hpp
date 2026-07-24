// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../text/FormatAs.hpp"
#include "../text/String.hpp"

#include <compare>
#include <cstddef>
#include <functional>
#include <optional>

namespace erbsland::network {

/// An opaque host name accepted by the platform resolver.
/// @tested{NetworkValueTest}
class HostName final {
public:
    // deletions
    HostName() = delete;

public: // operators
    /// Compare two host names.
    /// @param other The host name to compare with this host name.
    /// @return The strong lexical ordering between the names.
    [[nodiscard]] auto operator<=>(const HostName &other) const noexcept -> std::strong_ordering = default;

public: // accessors
    /// Get the host name.
    /// @return The retained host name.
    [[nodiscard]] auto name() const noexcept -> const text::String & { return _name; }

public: // conversion
    /// Get the host name as text.
    /// @return The retained host name.
    [[nodiscard]] auto toString() const -> text::String { return _name; }
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
    explicit HostName(text::String name) : _name{std::move(name)} {}

private:
    text::String _name; ///< The validated host name.
};

}

template <>
struct std::hash<erbsland::network::HostName> {
    auto operator()(const erbsland::network::HostName &value) const noexcept -> std::size_t { return value.toHash(); }
};

template <>
struct erbsland::text::FormatAsText<erbsland::network::HostName> : FormatAs<network::HostName, String> {
    [[nodiscard]] auto format(const network::HostName &value) const -> String { return value.toString(); }
};
