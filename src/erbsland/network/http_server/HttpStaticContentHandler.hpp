// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpMediaTypeMapping_fwd.hpp"
#include "HttpStaticContent_fwd.hpp"
#include "HttpStaticContentHandler_fwd.hpp"

#include "../impl/http/server/HttpServer_fwd.hpp"
#include "../impl/http/static/HttpStaticContentUse_fwd.hpp"

#include "../../path/Path.hpp"
#include "../../text/String.hpp"
#include "../../text/StringList.hpp"

#include <cstdint>
#include <mutex>

namespace erbsland::network {

/// Extensible server-level static-content source.
/// Methods may block, are invoked concurrently on server workers, and therefore must be thread-safe. Paths supplied
/// to `hasPath()` and `getContent()` are validated decoded-NFC relative paths and must be matched case-sensitively.
/// @seedoc{/reference/network/http_server}
/// @tested{HttpStaticContentTest}
class HttpStaticContentHandler {
    friend class impl::HttpServer;
    friend class impl::HttpStaticContentUse;

public:
    // defaults/deletions
    /// Release the handler configuration.
    virtual ~HttpStaticContentHandler() = default;
    HttpStaticContentHandler(const HttpStaticContentHandler &) = delete;
    HttpStaticContentHandler(HttpStaticContentHandler &&) = delete;
    auto operator=(const HttpStaticContentHandler &) -> HttpStaticContentHandler & = delete;
    auto operator=(HttpStaticContentHandler &&) -> HttpStaticContentHandler & = delete;

public: // content interface
    /// Probe whether this handler owns an exact relative path.
    /// Return false only for normal absence or ineligibility. A positive result is authoritative.
    [[nodiscard]] virtual auto hasPath(const path::Path &relativePath) const -> bool = 0;
    /// Create content for a path previously accepted by `hasPath()`.
    [[nodiscard]] virtual auto getContent(const path::Path &relativePath) const -> HttpStaticContentPtr = 0;

public: // accessors
    /// Get the decoded URL prefix matched on complete path segments.
    [[nodiscard]] auto urlPrefix() const -> text::String;
    /// Replace the decoded URL prefix matched on complete path segments.
    auto setUrlPrefix(text::String value) -> HttpStaticContentHandler &;
    /// Get the ordering priority; higher values are searched first.
    [[nodiscard]] auto priority() const noexcept -> std::int32_t;
    /// Set the ordering priority.
    auto setPriority(std::int32_t value) -> HttpStaticContentHandler &;
    /// Get the ordered index filenames.
    [[nodiscard]] auto indexFileNames() const -> text::StringList;
    /// Replace the ordered index filenames.
    auto setIndexFileNames(text::StringList value) -> HttpStaticContentHandler &;
    /// Get the shared media-type mapping.
    [[nodiscard]] auto mediaTypeMapping() const -> HttpMediaTypeMappingConstPtr;
    /// Replace the shared media-type mapping.
    auto setMediaTypeMapping(HttpMediaTypeMappingConstPtr value) -> HttpStaticContentHandler &;

protected:
    /// Create common static-content configuration.
    explicit HttpStaticContentHandler(text::String urlPrefix);

    /// Lock common configuration for an atomic derived-class access.
    [[nodiscard]] auto lockConfiguration() const -> std::unique_lock<std::mutex>;
    /// Require mutable configuration while holding the configuration lock.
    void verifyConfigurationMutable() const;

private:
    /// Validate and canonicalize a URL prefix.
    [[nodiscard]] static auto canonicalUrlPrefix(text::String value) -> text::String;
    /// Validate one index filename.
    static void verifyIndexFileName(const text::String &name);
    /// Freeze this handler and its current mapping for one server.
    void beginUse();
    /// Release one server's configuration freeze.
    void endUse() noexcept;

private:
    mutable std::mutex _mutex;                  ///< Protects concurrent configuration access and freezing.
    text::String _urlPrefix;                    ///< Decoded absolute URL prefix.
    std::int32_t _priority{};                   ///< Higher-first ordering priority.
    text::StringList _indexFileNames;           ///< Ordered index filenames.
    HttpMediaTypeMappingConstPtr _mediaTypeMap; ///< Shared suffix mapping.
    std::size_t _activeUseCount{};              ///< Servers currently retaining frozen configuration.
};

}
