// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpCookieSessionManagerData_fwd.hpp"

#include "../../../../event/Events_fwd.hpp"
#include "../../../../time/TimePoint.hpp"
#include "../../../http_server/HttpCookieSessionManagerOptions.hpp"
#include "../../../http_server/HttpServerSessionContext.hpp"
#include "../../../http_server/HttpServerSessionSelection.hpp"

#include <list>
#include <memory>
#include <optional>
#include <unordered_map>

namespace erbsland::network::impl {

/// Bounded owner-loop registry behind the public cookie session manager.
/// @tested{HttpCookieSessionManagerTest HttpServerLiveTest}
class HttpCookieSessionManagerData final : public std::enable_shared_from_this<HttpCookieSessionManagerData> {
private:
    /// One server-side session registry entry.
    struct Entry final {
        text::String identifier;                                      ///< Opaque cookie token.
        network::HttpServerSessionPtr session;                        ///< Server-owned logical session.
        time::TimePoint created;                                      ///< Absolute-lifetime origin.
        time::TimePoint lastUsed;                                     ///< Sliding-idle origin.
        bool secure{};                                                ///< Whether the issued cookie carried Secure.
        std::list<std::shared_ptr<Entry>>::iterator creationPosition; ///< Creation-order position.
        std::list<std::shared_ptr<Entry>>::iterator idlePosition;     ///< Idle-order position.
    };

public:
    /// Create registry data and validate its immutable options.
    explicit HttpCookieSessionManagerData(HttpCookieSessionManagerOptions options);

public:
    /// Resolve one request cookie or create a fresh registered session.
    [[nodiscard]] auto select(const HttpServerSessionContext &context) -> HttpServerSessionSelection;
    /// Remove a session and create its deletion-cookie fields.
    [[nodiscard]] auto invalidate(const network::HttpServerSessionPtr &session) -> HttpHeaders;

private:
    /// Validate cookie, lifetime, and capacity configuration.
    void validate() const;
    /// Parse one unique syntactically valid manager-cookie identifier.
    [[nodiscard]] auto requestIdentifier(const HttpHeaders &headers) const -> std::optional<text::String>;
    /// Create a fresh 256-bit unpadded base64url identifier.
    [[nodiscard]] static auto createIdentifier() -> text::String;
    /// Decide whether an issued cookie carries Secure.
    [[nodiscard]] auto usesSecureCookie(bool secureRequest) const noexcept -> bool;
    /// Build one Set-Cookie value without a Domain attribute.
    [[nodiscard]] auto cookieValue(const text::String &identifier, bool secure, bool deletion) const -> text::String;
    /// Remove all entries expired at the supplied owner-loop time.
    void removeExpired(time::TimePoint now);
    /// Capture and enforce exclusive server owner-loop use.
    void captureOwner(const event::EventsPtr &owner);
    /// Schedule bounded cleanup at the earliest ordered expiry.
    void scheduleCleanup();
    /// Move one entry to the newest idle position.
    void touch(const std::shared_ptr<Entry> &entry, time::TimePoint now);
    /// Remove one known entry from every ordered registry structure.
    void erase(const std::shared_ptr<Entry> &entry);

private:
    HttpCookieSessionManagerOptions _options;                          ///< Validated cookie and lifetime policy.
    std::unordered_map<text::String, std::shared_ptr<Entry>> _entries; ///< Identifier registry.
    std::list<std::shared_ptr<Entry>> _creation;                       ///< Absolute-expiry order.
    std::list<std::shared_ptr<Entry>> _idle;                           ///< Sliding-idle and eviction order.
    event::EventsPtr _ownerEvents;                                     ///< Exclusive server owner loop.
    bool _cleanupScheduled{};                                          ///< Whether an expiry callback is pending.
};

}
