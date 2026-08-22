// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpStaticContentHandler.hpp"
#include "HttpStaticFileHandler_fwd.hpp"

#include "../../path/Path.hpp"

namespace erbsland::network {

/// A static-content handler backed by one securely contained filesystem root.
/// @seedoc{/reference/network/http_server}
/// @tested{HttpStaticContentTest HttpServerLiveTest}
class HttpStaticFileHandler : public HttpStaticContentHandler {
public:
    /// Create a handler for one filesystem root and URL prefix.
    [[nodiscard]] static auto create(path::Path rootPath, text::String urlPrefix = text::String{"/"})
        -> HttpStaticFileHandlerPtr;

    // defaults
    ~HttpStaticFileHandler() override = default;

public:
    /// Get the configured filesystem root.
    [[nodiscard]] auto rootPath() const noexcept -> const path::Path & { return _rootPath; }

protected:
    /// Create a filesystem handler.
    HttpStaticFileHandler(path::Path rootPath, text::String urlPrefix);

private:
    path::Path _rootPath; ///< Configured filesystem root.
};

}
