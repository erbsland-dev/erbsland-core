// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../http/HttpHeaders.hpp"

#include "../../text/String.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// The result of renewing a manager-owned HTTP session identifier.
/// @tested{HttpCookieSessionManagerTest HttpServerLiveTest}
class HttpServerSessionRenewal final {
public:
    /// Create an invalid renewal result.
    HttpServerSessionRenewal() = default;
    /// Create a successful renewal result.
    HttpServerSessionRenewal(std::optional<text::String> identifier, HttpHeaders responseFields) :
        _identifier{std::move(identifier)}, _responseFields{std::move(responseFields)}, _valid{true} {}

public:
    /// Test whether the session identifier was renewed.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _valid; }
    /// Get the replacement identifier.
    [[nodiscard]] auto identifier() const noexcept -> const std::optional<text::String> & { return _identifier; }
    /// Get fields to append to the next response for this session.
    [[nodiscard]] auto responseFields() const noexcept -> const HttpHeaders & { return _responseFields; }

private:
    std::optional<text::String> _identifier; ///< Replacement manager identifier.
    HttpHeaders _responseFields;             ///< Manager-owned response fields.
    bool _valid{};                           ///< Whether renewal succeeded.
};

}
