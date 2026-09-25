// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Registry_fwd.hpp"

#include "../Filter.hpp"
#include "../Source.hpp"

#include <vector>

namespace erbsland::text::placeholder::impl {

/// Registry and dispatcher for placeholder sources and filters.
/// @tested{ReplacerTest ParserPlaceholderTest}
class Registry final {
    /// One registered source name and its provider.
    struct SourceEntry final {
        String name;        ///< Normalized source name.
        SourcePtr provider; ///< Provider implementing this name.
    };
    /// One registered filter name and its provider.
    struct FilterEntry final {
        String name;        ///< Normalized filter name.
        FilterPtr provider; ///< Provider implementing this name.
    };

public:
    /// Test whether placeholder expansion is active.
    [[nodiscard]] auto hasSources() const noexcept -> bool { return !_sources.empty(); }
    /// Find the provider registered for a normalized source name.
    [[nodiscard]] auto source(const String &name) const noexcept -> SourcePtr;
    /// Find the provider registered for a normalized filter name.
    [[nodiscard]] auto filter(const String &name) const noexcept -> FilterPtr;
    /// Add a source provider.
    void addSource(const SourcePtr &source);
    /// Remove a source provider by pointer identity.
    void removeSource(const SourcePtr &source) noexcept;
    /// Add a filter provider.
    void addFilter(const FilterPtr &filter);
    /// Remove a filter provider by pointer identity.
    void removeFilter(const FilterPtr &filter) noexcept;
    /// Resolve a source value.
    [[nodiscard]] auto resolve(const String &name, const String &parameter) const -> String;
    /// Apply a filter.
    [[nodiscard]] auto apply(const String &name, const String &parameter, const String &value) const -> String;

private:
    /// Normalize and validate a provider name.
    [[nodiscard]] static auto normalizeProviderName(const String &name, const String &kind) -> String;
    /// Check a pending source name for collisions.
    void verifySourceName(const String &name, const std::vector<String> &pending) const;
    /// Check a pending filter name for collisions.
    void verifyFilterName(const String &name, const std::vector<String> &pending) const;

private:
    std::vector<SourceEntry> _sources;
    std::vector<FilterEntry> _filters;
};

}
