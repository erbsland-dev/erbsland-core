// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRedirectConstraint.hpp"
#include "HttpClientRequest_fwd.hpp"

#include "../http/HttpResponseHead.hpp"
#include "../url/Url.hpp"

#include "../../unit/ItemCount.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// Context presented at a redirect-policy checkpoint.
/// @tested{HttpClientTest}
class HttpClientRedirectContext final {
public:
    /// Create one redirect context.
    HttpClientRedirectContext(
        HttpResponseHead responseHead,
        Url sourceUrl,
        std::optional<Url> targetUrl,
        unit::ItemCount followedCount,
        HttpClientRedirectConstraint constraint) noexcept :
        _responseHead{std::move(responseHead)},
        _sourceUrl{std::move(sourceUrl)},
        _targetUrl{std::move(targetUrl)},
        _followedCount{followedCount},
        _constraint{constraint} {}

public:
    /// Get the retained redirect response head.
    [[nodiscard]] auto responseHead() const noexcept -> const HttpResponseHead & { return _responseHead; }
    /// Get the URL used for the completed hop.
    [[nodiscard]] auto sourceUrl() const noexcept -> const Url & { return _sourceUrl; }
    /// Get the resolved target URL, if Location was valid.
    [[nodiscard]] auto targetUrl() const noexcept -> const std::optional<Url> & { return _targetUrl; }
    /// Get the number of redirects already followed.
    [[nodiscard]] constexpr auto followedCount() const noexcept -> unit::ItemCount { return _followedCount; }
    /// Test whether all non-overridable follow guards permit this hop.
    [[nodiscard]] constexpr auto canFollow() const noexcept -> bool {
        return _constraint == HttpClientRedirectConstraint::None;
    }
    /// Get the first hard constraint preventing a follow action.
    [[nodiscard]] constexpr auto constraint() const noexcept -> HttpClientRedirectConstraint { return _constraint; }
    /// Get the optional prepared request override supplied by the callback.
    [[nodiscard]] auto requestOverride() const noexcept -> const HttpClientRequestPtr & { return _requestOverride; }
    /// Use a prepared request from the same session for the next eligible hop.
    void setRequestOverride(HttpClientRequestPtr request) noexcept { _requestOverride = std::move(request); }

private:
    HttpResponseHead _responseHead;           ///< Redirect response head.
    Url _sourceUrl;                           ///< Completed-hop URL.
    std::optional<Url> _targetUrl;            ///< Resolved Location target.
    unit::ItemCount _followedCount;           ///< Completed redirect count.
    HttpClientRedirectConstraint _constraint; ///< Non-overridable guard result.
    HttpClientRequestPtr _requestOverride;    ///< Optional callback-provided prepared request.
};

}
