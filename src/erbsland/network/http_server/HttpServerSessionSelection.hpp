// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerSession_fwd.hpp"

#include "../http/HttpHeaders.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// The session and response fields selected for one request.
/// @tested{HttpCookieSessionManagerTest}
class HttpServerSessionSelection final {
public:
    /// Create an invalid selection.
    HttpServerSessionSelection() = default;
    /// Create a valid selection.
    HttpServerSessionSelection(
        HttpServerSessionPtr session,
        HttpHeaders responseFields = {},
        std::optional<text::String> reservedCookieName = {}) :
        _session{std::move(session)},
        _responseFields{std::move(responseFields)},
        _reservedCookieName{std::move(reservedCookieName)} {}

public:
    /// Test whether a non-null session was selected.
    [[nodiscard]] auto isValid() const noexcept -> bool { return _session != nullptr; }
    /// Get the selected logical session.
    [[nodiscard]] auto session() const noexcept -> const HttpServerSessionPtr & { return _session; }
    /// Get manager-owned fields appended to the eventual response.
    [[nodiscard]] auto responseFields() const noexcept -> const HttpHeaders & { return _responseFields; }
    /// Get a cookie name that application responses must not set.
    [[nodiscard]] auto reservedCookieName() const noexcept -> const std::optional<text::String> & {
        return _reservedCookieName;
    }

private:
    HttpServerSessionPtr _session;                   ///< Selected session.
    HttpHeaders _responseFields;                     ///< Fields appended to the eventual response.
    std::optional<text::String> _reservedCookieName; ///< Cookie name exclusively owned by the manager.
};

}
