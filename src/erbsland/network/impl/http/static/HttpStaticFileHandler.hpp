// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../http_server/HttpStaticFileHandler.hpp"

#include <optional>

namespace erbsland::network::impl {

/// Built-in filesystem static-content handler.
/// @notest{Internal implementation covered by HttpStaticContentTest and HttpServerLiveTest.}
class HttpStaticFileHandler final : public network::HttpStaticFileHandler {
private:
    /// One successfully resolved regular-file candidate.
    struct Candidate final {
        path::Path path;         ///< Physical file path.
        unit::ByteLength length; ///< Captured regular-file length.
    };

public:
    /// Create a built-in filesystem handler.
    HttpStaticFileHandler(path::Path rootPath, text::String urlPrefix);

    // defaults
    ~HttpStaticFileHandler() override = default;

public:
    [[nodiscard]] auto hasPath(const path::Path &relativePath) const -> bool override;
    [[nodiscard]] auto getContent(const path::Path &relativePath) const -> HttpStaticContentPtr override;

private:
    /// Resolve one exact case-sensitive regular file below the retained root.
    [[nodiscard]] auto resolveCandidate(const path::Path &relativePath) const -> std::optional<Candidate>;
    /// Test whether one physical path is strictly below the retained root.
    [[nodiscard]] auto isBelowRoot(const path::Path &candidate) const noexcept -> bool;
};

}
