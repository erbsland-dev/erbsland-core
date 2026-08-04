// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UrlData.hpp"

#include "../url/UrlFormatOptions.hpp"

#include "../../text/StringEditor.hpp"

#include <cstdint>

namespace erbsland::network::impl {

/// Serialize canonical URL text.
/// @tested{UrlTest}
class UrlWriter final {
public:
    /// Create a writer for validated URL data.
    UrlWriter(const UrlData &data, UrlFormatOptions options) : _data{data}, _options{options} {}
    /// Serialize the configured URL.
    [[nodiscard]] auto write() -> text::String;

private:
    /// URL component encoding context.
    enum class Component : uint8_t { UserInfo, Path, Query, Fragment };

private:
    /// Write an authority according to its parsing state.
    void writeAuthority();
    /// Write a parsed authority from structured components.
    void writeParsedAuthority();
    /// Write a host using the requested IDNA representation.
    void writeHost();
    /// Percent-encode one decoded URL component.
    void writeEncoded(const text::String &value, Component component);
    /// Write one percent-encoded byte.
    void writeEncodedByte(uint8_t value);
    /// Test whether a character is permitted unescaped in a component.
    [[nodiscard]] auto isAllowed(text::Char character, Component component) const noexcept -> bool;
    /// Get the inferred port for the URL scheme.
    [[nodiscard]] auto defaultPort() const noexcept -> Port;

private:
    const UrlData &_data;       ///< URL to serialize.
    UrlFormatOptions _options;  ///< Formatting options.
    text::StringEditor _result; ///< Output buffer.
};

}
