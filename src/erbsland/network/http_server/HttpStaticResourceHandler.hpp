// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpStaticContentHandler.hpp"
#include "HttpStaticResourceHandler_fwd.hpp"

#include "../../resource/Resources_fwd.hpp"
#include "../../unit/ByteCount.hpp"

namespace erbsland::network {

/// A static-content handler backed by one compiled or custom resource identifier.
/// @seedoc{/reference/network/http_server}
/// @tested{HttpStaticContentTest HttpServerLiveTest}
class HttpStaticResourceHandler : public HttpStaticContentHandler {
public:
    /// Default per-resource logical-content limit matching the resource compiler.
    static constexpr auto cDefaultMaximumContentLength = unit::ByteCount{1024U * 1024U};

public:
    /// Create a handler retaining an explicit resource provider.
    [[nodiscard]] static auto create(
        resource::ResourcesConstPtr resources, text::String identifier, text::String urlPrefix = text::String{"/"})
        -> HttpStaticResourceHandlerPtr;
    /// Create a handler retaining the application's resource provider.
    [[nodiscard]] static auto create(text::String identifier, text::String urlPrefix = text::String{"/"})
        -> HttpStaticResourceHandlerPtr;

    // defaults
    ~HttpStaticResourceHandler() override = default;

public: // accessors
    /// Get the resource provider.
    [[nodiscard]] auto resources() const noexcept -> const resource::Resources & { return *_retainedResources; }
    /// Get the exact portable resource identifier.
    [[nodiscard]] auto identifier() const noexcept -> const text::String & { return _identifier; }
    /// Get the per-resource logical-content maximum.
    [[nodiscard]] auto maximumContentLength() const noexcept -> unit::ByteCount;
    /// Set the positive finite per-resource logical-content maximum.
    auto setMaximumContentLength(unit::ByteCount value) -> HttpStaticResourceHandler &;

protected:
    /// Create a retained-provider handler.
    HttpStaticResourceHandler(resource::ResourcesConstPtr resources, text::String identifier, text::String urlPrefix);
    /// Validate one portable resource identifier.
    static void verifyIdentifier(const text::String &identifier);

    /// Access the retained provider for content objects.
    [[nodiscard]] auto retainedResources() const noexcept -> const resource::ResourcesConstPtr & {
        return _retainedResources;
    }

private:
    resource::ResourcesConstPtr _retainedResources;                      ///< Retained provider.
    text::String _identifier;                                            ///< Exact resource identifier.
    unit::ByteCount _maximumContentLength{cDefaultMaximumContentLength}; ///< Logical content bound.
};

}
