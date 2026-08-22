// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UrlData.hpp"

#include "../../../unit/ByteLength.hpp"
#include "../../url/UrlParseOptions.hpp"

#include <memory>
#include <utility>

namespace erbsland::network::impl {

/// Resolve one URI reference against canonical URL data using RFC 3986 section 5.2.
/// @tested{UrlTest}
class UrlResolver final {
public:
    /// Create a resolver for one base and reference.
    UrlResolver(const UrlData &base, text::String reference, UrlParseOptions options) :
        _base{base}, _reference{std::move(reference)}, _options{options} {}
    /// Resolve the configured reference.
    [[nodiscard]] auto resolve() const -> std::shared_ptr<UrlData>;

private:
    /// Parsed relative-reference components.
    struct Reference {
        text::String path;     ///< Encoded path text.
        text::String query;    ///< Encoded query text.
        text::String fragment; ///< Encoded fragment text.
        bool hasQuery{};       ///< A query delimiter was present.
        bool hasFragment{};    ///< A fragment delimiter was present.
    };

private:
    /// Test whether the reference starts with a syntactically valid scheme.
    [[nodiscard]] auto hasScheme() const noexcept -> bool;
    /// Test whether the base supports hierarchical relative resolution.
    [[nodiscard]] auto hasHierarchicalBase() const noexcept -> bool;
    /// Parse a reference without scheme or authority.
    [[nodiscard]] auto parseRelative() const -> Reference;
    /// Resolve parsed relative components against the base.
    [[nodiscard]] auto resolveRelative(const Reference &reference) const -> std::shared_ptr<UrlData>;
    /// Parse an absolute or scheme-relative reference and normalize its path.
    [[nodiscard]] auto parseAbsolute(const text::String &text) const -> std::shared_ptr<UrlData>;
    /// Merge a relative path with the base path.
    [[nodiscard]] auto mergePath(const text::String &path) const -> text::String;
    /// Remove dot segments from a decoded path.
    [[nodiscard]] static auto removeDotSegments(text::String path) -> text::String;
    /// Remove the last segment from an output path.
    [[nodiscard]] static auto removeLastSegment(const text::String &path) -> text::String;
    /// Remove an ASCII byte prefix from text.
    [[nodiscard]] static auto removePrefix(const text::String &text, unit::ByteLength length) -> text::String;
    /// Concatenate two strings.
    [[nodiscard]] static auto join(const text::String &first, const text::String &second) -> text::String;

private:
    const UrlData &_base;     ///< Canonical base URL data.
    text::String _reference;  ///< Reference text.
    UrlParseOptions _options; ///< Active safety limits.
};

}
