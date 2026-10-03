// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeZoneModel.hpp"

#include <erbsland/render/Value.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEditor.hpp>

#include <utility>
#include <vector>

namespace tool::time_zone {

/// Render all runtime translation units from a validated, source-derived time-zone model.
/// @tested{TimeZoneDataToolTest}
class TimeZoneRenderer final {
    /// Derived abbreviation tables for one zone, prior to textual rendering.
    struct ZoneAbbreviations {
        el::StringMap<TimeZoneModel::RuleSetPtr> usedRuleSets; ///< Ordered rules used by zone continuations.
        std::vector<std::vector<uint32_t>> abbreviationLists;  ///< Dynamic text IDs for each continuation.
        std::vector<uint32_t> fixedIds;                        ///< Fixed text IDs for each continuation.
        std::vector<uint32_t> packedIds;                       ///< Deduplicated dynamic abbreviation sequence.
        std::vector<uint32_t> packedOffsets; ///< One-based sequence offsets, or zero for fixed entries.
        std::vector<uint32_t> textIds;       ///< Sorted nonzero IDs used by the zone.
    };

public:
    /// Render every output in memory before the command writes any file.
    /// @param model The validated tzdb and CLDR model.
    explicit TimeZoneRenderer(const TimeZoneModel &model);

public: // accessors
    /// Get generated filename and content pairs.
    [[nodiscard]] auto outputs() const noexcept -> const std::vector<std::pair<el::String, el::String>> & {
        return _outputs;
    }

private:
    /// Render compact names and abbreviation text.
    void renderTexts();
    /// Render compact daylight-saving rules.
    void renderRules();
    /// Render primary zone factories and dispatch.
    void renderZones();
    /// Render version and CLDR Windows mapping.
    void renderMetadata();
    /// Derive fixed and dynamic abbreviations from the source rule sets.
    auto zoneAbbreviations(const TimeZoneModel::Zone &zone) const -> ZoneAbbreviations;
    /// Pack overlapping abbreviation sequences and collect their text IDs.
    static void packAbbreviations(ZoneAbbreviations &data);
    /// Translate each zone continuation using the prepared local tables.
    auto zoneContinuations(
        const TimeZoneModel::Zone &zone, const ZoneAbbreviations &data, const el::StringList &ruleNames) const
        -> el::text::render::ValueList;
    /// Render one zone factory.
    [[nodiscard]] auto zoneContext(const TimeZoneModel::Zone &zone) const -> el::text::render::ValueMap;
    /// Add a complete output file.
    void add(el::String name, const el::String &body);
    /// Format an unsigned number in hexadecimal.
    [[nodiscard]] static auto hex(uint64_t value, uint32_t digits) -> el::String;

private:
    const TimeZoneModel &_model;                             ///< Validated source data.
    std::vector<std::pair<el::String, el::String>> _outputs; ///< All rendered files.
};

}
