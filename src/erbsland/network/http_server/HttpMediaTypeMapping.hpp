// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpMediaTypeMapping_fwd.hpp"
#include "HttpStaticContentHandler_fwd.hpp"

#include "../http/HttpMediaType.hpp"

#include "../../text/String.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

#include <mutex>
#include <utility>
#include <vector>

namespace erbsland::network {

/// A bounded suffix-to-media-type mapping for static HTTP content.
/// Suffix matching is ASCII-case-insensitive, and the longest matching suffix wins.
/// @seedoc{/reference/network/http_server}
/// @tested{HttpStaticContentTest}
class HttpMediaTypeMapping final {
    friend class HttpStaticContentHandler;

private:
    /// One normalized suffix mapping.
    using Entry = std::pair<text::String, HttpMediaType>;

public:
    /// Maximum number of custom suffix mappings.
    static constexpr auto cMaximumSuffixCount = unit::ItemCount{256U};
    /// Maximum length of one suffix.
    static constexpr auto cMaximumSuffixLength = unit::ByteLength{64U};

public:
    /// Create an empty mapping with an application/octet-stream fallback.
    [[nodiscard]] static auto create() -> HttpMediaTypeMappingPtr;
    /// Access the immutable built-in mapping.
    [[nodiscard]] static auto defaultMapping() -> HttpMediaTypeMappingConstPtr;

    // defaults/deletions
    ~HttpMediaTypeMapping() = default;
    HttpMediaTypeMapping(const HttpMediaTypeMapping &) = delete;
    HttpMediaTypeMapping(HttpMediaTypeMapping &&) = delete;
    auto operator=(const HttpMediaTypeMapping &) -> HttpMediaTypeMapping & = delete;
    auto operator=(HttpMediaTypeMapping &&) -> HttpMediaTypeMapping & = delete;

public: // accessors
    /// Get the fallback used when no suffix matches.
    [[nodiscard]] auto fallbackMediaType() const -> HttpMediaType;
    /// Replace the fallback media type.
    auto setFallbackMediaType(HttpMediaType mediaType) -> HttpMediaTypeMapping &;
    /// Replace the fallback from validated media-type text.
    auto setFallbackMediaType(text::String mediaType) -> HttpMediaTypeMapping &;
    /// Add or replace one suffix mapping.
    auto setSuffix(text::String suffix, HttpMediaType mediaType) -> HttpMediaTypeMapping &;
    /// Add or replace one suffix mapping from validated media-type text.
    auto setSuffix(text::String suffix, text::String mediaType) -> HttpMediaTypeMapping &;
    /// Remove one suffix mapping.
    auto removeSuffix(const text::String &suffix) -> HttpMediaTypeMapping &;
    /// Remove every suffix mapping while preserving the fallback.
    auto clear() -> HttpMediaTypeMapping &;

public: // lookup/copy
    /// Resolve a filename or relative path to its configured media type.
    [[nodiscard]] auto mediaType(const text::String &path) const -> HttpMediaType;
    /// Create a mutable independent copy.
    [[nodiscard]] auto copy() const -> HttpMediaTypeMappingPtr;

private:
    /// Create an empty mapping.
    HttpMediaTypeMapping();
    /// Validate and normalize one suffix.
    [[nodiscard]] static auto normalizedSuffix(text::String suffix) -> text::String;
    /// Require one valid media type.
    static void verifyMediaType(const HttpMediaType &mediaType);
    /// Require mutable mapping configuration while holding `_mutex`.
    void verifyMutable() const;
    /// Freeze this mapping for one active handler/server use.
    void beginUse() const;
    /// Release one active handler/server use.
    void endUse() const noexcept;

private:
    mutable std::mutex _mutex;             ///< Protects concurrent configuration, freezing, and lookups.
    std::vector<Entry> _entries;           ///< Unique normalized suffix mappings.
    HttpMediaType _fallback;               ///< Fallback for unmatched paths.
    mutable std::size_t _activeUseCount{}; ///< Active handlers retained by starting or active servers.
};

}
