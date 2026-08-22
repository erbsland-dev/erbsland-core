// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpStaticContentUse_fwd.hpp"

#include "../../../http_server/HttpStaticContentHandler_fwd.hpp"

#include <vector>

namespace erbsland::network::impl {

/// Shared active-use lease for one server's original static-content handlers.
/// @notest{Internal implementation covered by HttpStaticContentTest.}
class HttpStaticContentUse final {
public:
    /// Freeze every handler, rolling back if acquisition fails.
    [[nodiscard]] static auto create(const std::vector<HttpStaticContentHandlerPtr> &handlers)
        -> std::shared_ptr<HttpStaticContentUse>;

    /// Release every handler freeze.
    ~HttpStaticContentUse();

    // defaults/deletions
    HttpStaticContentUse(const HttpStaticContentUse &) = delete;
    HttpStaticContentUse(HttpStaticContentUse &&) = delete;
    auto operator=(const HttpStaticContentUse &) -> HttpStaticContentUse & = delete;
    auto operator=(HttpStaticContentUse &&) -> HttpStaticContentUse & = delete;

private:
    /// Retain handlers whose freezes were acquired.
    explicit HttpStaticContentUse(std::vector<HttpStaticContentHandlerPtr> handlers) noexcept;

private:
    std::vector<HttpStaticContentHandlerPtr> _handlers; ///< Frozen handlers, including duplicate registrations.
};

}
