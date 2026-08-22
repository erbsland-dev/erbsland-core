// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::network {

/// Finite storage limits for an HTTP client cookie jar.
/// @tested{HttpCookieJarTest}
class HttpCookieJarOptions final {
public:
    static constexpr auto cDefaultMaximumCookieLength = unit::ByteLength{4U * 1024U};
    static constexpr auto cDefaultMaximumCookiesPerRegistrableDomain = unit::ItemCount{180U};
    static constexpr auto cDefaultMaximumCookies = unit::ItemCount{3000U};

public:
    /// Get the maximum serialized length of one cookie.
    [[nodiscard]] constexpr auto maximumCookieLength() const noexcept -> unit::ByteLength { return _maximumLength; }
    /// Set the maximum serialized length of one cookie.
    constexpr auto setMaximumCookieLength(const unit::ByteLength value) noexcept -> HttpCookieJarOptions & {
        _maximumLength = value;
        return *this;
    }
    /// Get the maximum number of cookies retained per registrable domain.
    [[nodiscard]] constexpr auto maximumCookiesPerRegistrableDomain() const noexcept -> unit::ItemCount {
        return _maximumPerDomain;
    }
    /// Set the maximum number of cookies retained per registrable domain.
    constexpr auto setMaximumCookiesPerRegistrableDomain(const unit::ItemCount value) noexcept
        -> HttpCookieJarOptions & {
        _maximumPerDomain = value;
        return *this;
    }
    /// Get the global maximum number of cookies.
    [[nodiscard]] constexpr auto maximumCookies() const noexcept -> unit::ItemCount { return _maximum; }
    /// Set the global maximum number of cookies.
    constexpr auto setMaximumCookies(const unit::ItemCount value) noexcept -> HttpCookieJarOptions & {
        _maximum = value;
        return *this;
    }

private:
    unit::ByteLength _maximumLength{cDefaultMaximumCookieLength};
    unit::ItemCount _maximumPerDomain{cDefaultMaximumCookiesPerRegistrableDomain};
    unit::ItemCount _maximum{cDefaultMaximumCookies};
};

}
