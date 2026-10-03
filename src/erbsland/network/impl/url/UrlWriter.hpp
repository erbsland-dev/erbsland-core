// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UrlData.hpp"

#include "../../../text/StringEditor.hpp"
#include "../../url/UrlFormatOptions.hpp"

namespace erbsland::network::impl {

/// Serialize canonical URL text.
/// @tested{UrlTest}
class UrlWriter final {
public:
    /// Create a writer for validated URL data.
    UrlWriter(const UrlData &data, UrlFormatOptions options) : _data{data}, _options{options} {}
    /// Serialize the configured URL.
    [[nodiscard]] auto write() -> text::String;
    /// Serialize an HTTP origin-form request target without a fragment.
    [[nodiscard]] auto writeRequestTarget() -> text::String;
    /// Serialize the HTTP Host field value.
    [[nodiscard]] auto writeHostField() -> text::String;

private:
    /// Write an authority according to its parsing state.
    void writeAuthority();
    /// Write a parsed authority from structured components.
    void writeParsedAuthority();
    /// Write a host using the requested IDNA representation.
    void writeHost();
    /// Get the inferred port for the URL scheme.
    [[nodiscard]] auto defaultPort() const noexcept -> Port;

private:
    const UrlData &_data;       ///< URL to serialize.
    UrlFormatOptions _options;  ///< Formatting options.
    text::StringEditor _result; ///< Output buffer.
};

}
