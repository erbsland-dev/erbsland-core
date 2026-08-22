// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../unit/ByteLength.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::network {

/// Automatic HTTP client response aggregation and media policy.
/// @tested{HttpClientTest}
class HttpClientResponseOptions final {
public:
    /// Default automatic response-body aggregation limit.
    static constexpr auto cDefaultMaximumBodyLength = unit::ByteLength{1024U * 1024U};

public:
    /// Get the automatic response-body aggregation limit.
    [[nodiscard]] auto maximumBodyLength() const noexcept -> unit::ByteLength { return _maximumBodyLength; }
    /// Set the positive finite automatic response-body aggregation limit.
    auto setMaximumBodyLength(unit::ByteLength value) noexcept -> HttpClientResponseOptions & {
        _maximumBodyLength = value;
        return *this;
    }
    /// Get explicitly configured accepted media-type patterns.
    /// No value selects the handler-family defaults; an empty list disables filtering.
    [[nodiscard]] auto acceptedContentTypes() const noexcept -> const std::optional<std::vector<text::String>> & {
        return _acceptedContentTypes;
    }
    /// Replace accepted media-type patterns; an empty list disables filtering.
    auto setAcceptedContentTypes(std::vector<text::String> value) noexcept -> HttpClientResponseOptions & {
        _acceptedContentTypes = std::move(value);
        return *this;
    }

private:
    unit::ByteLength _maximumBodyLength{cDefaultMaximumBodyLength}; ///< Automatic aggregate bound.
    std::optional<std::vector<text::String>> _acceptedContentTypes; ///< Optional media-type patterns.
};

}
