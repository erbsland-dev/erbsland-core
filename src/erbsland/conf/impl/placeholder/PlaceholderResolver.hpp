// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PlaceholderResolver_fwd.hpp"

#include "../../PlaceholderFilter.hpp"
#include "../../PlaceholderSource.hpp"

#include <vector>

namespace erbsland::conf::impl::placeholder {

/// Registry and dispatcher for placeholder sources and filters.
/// @tested{ParserPlaceholderTest}
class PlaceholderResolver final {
    /// One registered source name and its provider.
    struct SourceEntry final {
        text::String name;             ///< Normalized source name.
        PlaceholderSourcePtr provider; ///< Provider implementing this name.
    };
    /// One registered filter name and its provider.
    struct FilterEntry final {
        text::String name;             ///< Normalized filter name.
        PlaceholderFilterPtr provider; ///< Provider implementing this name.
    };

public:
    /// Test whether placeholder expansion is active.
    [[nodiscard]] auto hasSources() const noexcept -> bool { return !_sources.empty(); }
    /// Find the provider registered for a normalized source name.
    [[nodiscard]] auto source(const text::String &name) const noexcept -> PlaceholderSourcePtr;
    /// Add a source provider.
    void addSource(const PlaceholderSourcePtr &source);
    /// Remove a source provider by pointer identity.
    void removeSource(const PlaceholderSourcePtr &source) noexcept;
    /// Add a filter provider.
    void addFilter(const PlaceholderFilterPtr &filter);
    /// Remove a filter provider by pointer identity.
    void removeFilter(const PlaceholderFilterPtr &filter) noexcept;
    /// Resolve a source value.
    [[nodiscard]] auto resolve(const text::String &name, const text::String &parameter) const -> text::String;
    /// Apply a filter.
    [[nodiscard]] auto apply(const text::String &name, const text::String &parameter, const text::String &value) const
        -> text::String;

private:
    /// Normalize and validate a provider name.
    [[nodiscard]] static auto normalizeProviderName(const text::String &name, const text::String &kind) -> text::String;
    /// Check a pending source name for collisions.
    void verifySourceName(const text::String &name, const std::vector<text::String> &pending) const;
    /// Check a pending filter name for collisions.
    void verifyFilterName(const text::String &name, const std::vector<text::String> &pending) const;

private:
    std::vector<SourceEntry> _sources;
    std::vector<FilterEntry> _filters;
};

}
