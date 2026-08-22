// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../text/String.hpp"

#include <vector>

namespace erbsland::network::impl {

/// Parsed origin-form request target for server routing.
/// @tested{HttpRoutesTest}
class HttpRequestTarget final {
public:
    /// Parse and normalize one strict origin-form target.
    explicit HttpRequestTarget(const text::String &target);

public:
    /// Get the decoded NFC path without query.
    [[nodiscard]] auto path() const noexcept -> const text::String & { return _path; }
    /// Get the exact query without its question mark.
    [[nodiscard]] auto query() const noexcept -> const text::String & { return _query; }
    /// Get decoded segments split before percent decoding.
    [[nodiscard]] auto segments() const noexcept -> const std::vector<text::String> & { return _segments; }

private:
    /// Strictly percent-decode and NFC-normalize one segment.
    [[nodiscard]] static auto decodeSegment(const text::String &segment) -> text::String;

private:
    text::String _path;                  ///< Decoded NFC path.
    text::String _query;                 ///< Exact query without question mark.
    std::vector<text::String> _segments; ///< Decoded segments.
};

}
