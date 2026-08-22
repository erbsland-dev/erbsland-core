// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../http_server/HttpStaticResourceHandler.hpp"

namespace erbsland::network::impl {

/// Built-in resource-provider static-content handler.
/// @notest{Internal implementation covered by HttpStaticContentTest and HttpServerLiveTest.}
class HttpStaticResourceHandler final : public network::HttpStaticResourceHandler {
public:
    /// Create a retained-provider implementation.
    HttpStaticResourceHandler(resource::ResourcesConstPtr resources, text::String identifier, text::String urlPrefix);
    /// Create an application-provider implementation.
    HttpStaticResourceHandler(const resource::Resources &resources, text::String identifier, text::String urlPrefix);

    // defaults
    ~HttpStaticResourceHandler() override = default;

public:
    [[nodiscard]] auto hasPath(const path::Path &relativePath) const -> bool override;
    [[nodiscard]] auto getContent(const path::Path &relativePath) const -> HttpStaticContentPtr override;

private:
    /// Convert one relative path to an exact portable resource path.
    [[nodiscard]] static auto portablePath(const path::Path &relativePath) -> text::String;
};

}
