// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Decomposition.hpp"
#include "UnicodeSource.hpp"

#include <erbsland/render/Value.hpp>
#include <erbsland/String.hpp>

#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace tool::unicode {

/// Derive compact Unicode normalization tables from verified source records.
/// @tested{UnicodeDataToolTest}
class UnicodeNormalizationTables final {
public:
    /// Parse, validate, and encode the complete snapshot.
    /// @param source Verified Unicode source.
    explicit UnicodeNormalizationTables(const UnicodeSource &source);

public: // rendering
    /// Render the runtime normalization translation unit.
    [[nodiscard]] auto render() const -> el::String;

public: // encoded data
    /// Get source-derived canonical decomposition bytes.
    [[nodiscard]] auto decompositionData() const noexcept -> const std::vector<uint8_t> & { return _decompositionData; }
    /// Get source-derived combining-class bytes.
    [[nodiscard]] auto combiningData() const noexcept -> const std::vector<uint8_t> & { return _combiningData; }
    /// Get source-derived composition bytes.
    [[nodiscard]] auto compositionData() const noexcept -> const std::vector<uint8_t> & { return _compositionData; }

private:
    /// Parse one hexadecimal UCD scalar field.
    [[nodiscard]] static auto parseHex(const el::String &text) -> uint32_t;
    /// Append one canonical unsigned LEB128 value.
    static void encodeUnsigned(std::vector<uint8_t> &output, uint32_t value);
    /// Pack a page and its byte-stream offset.
    [[nodiscard]] static auto packPage(uint32_t page, std::size_t offset) -> uint32_t;
    /// Get the combining class, defaulting to zero.
    [[nodiscard]] auto combiningClass(uint32_t codePoint) const -> uint8_t;
    /// Resolve the fully expanded decomposition for one scalar.
    [[nodiscard]] auto expanded(uint32_t codePoint, bool compatibility, std::set<uint32_t> &active) const
        -> std::vector<uint32_t>;
    /// Determine the maximum recursive decomposition depth.
    [[nodiscard]] auto depth(uint32_t codePoint, bool compatibility, std::set<uint32_t> &active) const -> uint32_t;
    /// Determine the maximum class-zero composition chain.
    [[nodiscard]] auto compositionDepth(uint32_t starter, std::set<uint32_t> &active) const -> uint32_t;
    /// Validate the fixed runtime buffers against this snapshot.
    void validateRuntimeLimits() const;
    /// Encode source data into the three sparse page streams.
    void encode();
    /// Prepare one array for the render template.
    [[nodiscard]] static auto arrayContext(
        const el::String &name, const el::String &type, const std::vector<uint32_t> &values, uint32_t digits)
        -> el::text::render::ValueMap;

private:
    std::map<uint32_t, Decomposition> _decompositions;                            ///< Direct source mappings.
    std::map<uint32_t, uint8_t> _combining;                                       ///< Nonzero combining classes.
    std::map<uint32_t, std::vector<std::pair<uint32_t, uint32_t>>> _compositions; ///< Canonical composition pairs.
    std::vector<uint32_t> _decompositionPages;                                    ///< Packed sparse page directory.
    std::vector<uint8_t> _decompositionData;                                      ///< Encoded direct mappings.
    std::vector<uint32_t> _combiningPages;                                        ///< Packed combining-class directory.
    std::vector<uint8_t> _combiningData;                                          ///< Encoded combining classes.
    std::vector<uint32_t> _compositionPages;                                      ///< Packed starter directory.
    std::vector<uint8_t> _compositionData;                                        ///< Encoded composition pairs.
};

}
