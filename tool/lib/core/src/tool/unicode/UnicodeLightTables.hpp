// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LightDelta.hpp"
#include "LightRange.hpp"
#include "UnicodeSource.hpp"

#include <erbsland/String.hpp>

#include <cstdint>
#include <vector>

namespace tool::unicode {

/// Derive and render Unicode Light tables entirely from a verified UCD snapshot.
/// @tested{UnicodeDataToolTest}
class UnicodeLightTables final {
public:
    /// Build the dense values and compress them to the runtime representation.
    /// @param source Verified Unicode source.
    explicit UnicodeLightTables(const UnicodeSource &source);

public: // accessors
    /// Get the compressed ranges, including the terminal sentinel.
    [[nodiscard]] auto ranges() const noexcept -> const std::vector<LightRange> & { return _ranges; }
    /// Get the unique case-mapping triples.
    [[nodiscard]] auto deltas() const noexcept -> const std::vector<LightDelta> & { return _deltas; }

public: // rendering
    /// Render the runtime data translation unit.
    [[nodiscard]] auto renderData() const -> el::String;
    /// Render the runtime Unicode version translation unit.
    /// @param version Version verified against the UCD snapshot.
    [[nodiscard]] static auto renderVersion(const el::String &version) -> el::String;

private:
    /// Parse a single UCD hexadecimal scalar field.
    [[nodiscard]] static auto parseHex(const el::String &text) -> uint32_t;
    /// Resolve the category name used by the runtime enum.
    [[nodiscard]] static auto categoryName(const el::String &category) -> el::String;
    /// Find the range containing one code point.
    [[nodiscard]] auto entryAt(uint32_t codePoint) const -> LightRange;

private:
    std::vector<LightRange> _ranges; ///< Compressed source-derived entries.
    std::vector<LightDelta> _deltas; ///< Unique mapping triples.
};

}
