// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../text/StringList.hpp"
#include "../../unit/ByteCount.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace erbsland::network {

/// Body limits and media filters for an automatic HTTP server route.
/// @tested{HttpRoutesTest HttpServerLiveTest}
class HttpServerRouteOptions final {
public:
    /// Default automatic request-body aggregation limit.
    static constexpr auto cDefaultMaximumBodyLength = unit::ByteCount{1024U * 1024U};

public:
    /// Get the automatic request-body aggregation limit.
    [[nodiscard]] auto maximumBodyLength() const noexcept -> unit::ByteCount { return _maximumBodyLength; }
    /// Set the positive finite automatic request-body aggregation limit.
    auto setMaximumBodyLength(unit::ByteCount value) noexcept -> HttpServerRouteOptions & {
        _maximumBodyLength = value;
        return *this;
    }
    /// Get explicitly configured accepted media-type patterns.
    /// No value selects the route-kind defaults; an empty list disables filtering.
    [[nodiscard]] auto acceptedContentTypes() const noexcept -> const std::optional<text::StringList> & {
        return _acceptedContentTypes;
    }
    /// Replace accepted media-type patterns; an empty list disables filtering.
    auto setAcceptedContentTypes(text::StringList value) noexcept -> HttpServerRouteOptions & {
        _acceptedContentTypes = std::move(value);
        return *this;
    }

private:
    unit::ByteCount _maximumBodyLength{cDefaultMaximumBodyLength}; ///< Automatic aggregate bound.
    std::optional<text::StringList> _acceptedContentTypes;         ///< Explicit media-type patterns.
};

}
