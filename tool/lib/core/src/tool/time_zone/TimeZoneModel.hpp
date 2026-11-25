// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TimeZoneSource.hpp"

#include <erbsland/String.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSet.hpp>
#include <erbsland/xml/XmlNode.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

namespace tool::time_zone {

/// Parsed tzdb model and source-derived compact data used by the renderer.
/// @tested{TimeZoneDataToolTest}
class TimeZoneModel final {
public:
    /// One parsed daylight-saving rule.
    struct Rule {
        el::String name;    ///< Rule-set name.
        int yearFrom{};     ///< First active year.
        el::String yearTo;  ///< Last active year or max.
        el::String month;   ///< Month abbreviation.
        el::String day;     ///< Day expression.
        el::String at;      ///< Transition time and reference.
        el::String save;    ///< Saving offset.
        el::String letters; ///< Abbreviation letter token.
    };
    /// One named set of daylight-saving rules.
    struct RuleSet {
        el::String name;                         ///< Source name.
        std::vector<Rule> rules;                 ///< Rules in source order.
        uint32_t id{};                           ///< Sorted runtime index.
    };
    using RuleSetPtr = std::shared_ptr<RuleSet>; ///< A rule set owned by the model and updated during parsing.
    /// One continuation line of a time-zone history.
    struct ZoneEntry {
        el::String standardOffset; ///< Base UTC offset.
        el::String ruleName;       ///< Rule-set name or fixed save.
        el::String format;         ///< Abbreviation format.
        el::String until;          ///< Optional transition boundary.
    };
    /// One primary IANA zone.
    struct Zone {
        el::String name;                   ///< Canonical source name.
        el::String region;                 ///< Source region file.
        std::vector<ZoneEntry> entries;    ///< History in source order.
        uint32_t id{};                     ///< Sorted runtime identifier.
        el::String functionName;           ///< Unique generated factory name.
    };
    using ZonePtr = std::shared_ptr<Zone>; ///< A zone owned by the model and updated during parsing.
    /// One parsed offset with time reference.
    struct TimeValue {
        int seconds{};        ///< Signed seconds.
        uint8_t reference{1}; ///< UTC=0, wall=1, standard=2.
    };
    /// One transition used to infer an UNTIL boundary offset.
    struct Transition {
        int64_t seconds{};   ///< Local civil timestamp since Core epoch.
        uint8_t reference{}; ///< Transition reference.
        int saveMinutes{};   ///< Post-transition saving.
    };

public:
    /// Parse and validate one pinned time-zone snapshot.
    /// @param source Verified tzdb and CLDR source texts.
    explicit TimeZoneModel(const TimeZoneSource &source);

public: // accessors
    /// Get named rule sets sorted by name.
    [[nodiscard]] auto ruleSets() const noexcept -> const el::StringMap<RuleSetPtr> & { return _ruleSets; }
    /// Get primary zones sorted by name.
    [[nodiscard]] auto zones() const noexcept -> const el::StringMap<ZonePtr> & { return _zones; }
    /// Get aliases and their primary names.
    [[nodiscard]] auto links() const noexcept -> const el::StringMap<el::String> & { return _links; }
    /// Get sorted text fragments, including the empty ID-zero entry.
    [[nodiscard]] auto texts() const noexcept -> const el::StringList & { return _texts; }
    /// Get sorted CLDR world-territory Windows mappings.
    [[nodiscard]] auto windowsMappings() const noexcept -> const std::vector<std::pair<el::String, el::String>> & {
        return _windowsMappings;
    }
    /// Get the source-derived tzdb release year.
    [[nodiscard]] auto versionYear() const noexcept -> int { return _versionYear; }
    /// Get the source-derived tzdb release revision.
    [[nodiscard]] auto versionRevision() const noexcept -> int { return _versionRevision; }
    /// Get the source region names in generation order.
    [[nodiscard]] static auto regions() -> el::StringList;
    /// Get one canonical zone by its name or alias.
    [[nodiscard]] auto zoneFor(const el::String &name) const -> const Zone &;
    /// Get the source-derived text fragment ID, or zero for empty text.
    [[nodiscard]] auto textId(const el::String &text) const -> uint32_t;
    /// Create the four packed IDs for a zone name.
    [[nodiscard]] auto zoneName(const el::String &name) const -> std::array<uint32_t, 4>;
    /// Get all possible abbreviations of a zone.
    [[nodiscard]] auto abbreviations(const Zone &zone) const -> el::StringSet;
    /// Resolve one abbreviation pattern.
    [[nodiscard]] auto abbreviation(const ZoneEntry &entry, const el::String &letters, int offsetSeconds) const
        -> el::String;
    /// Test whether the entry references a named rule set.
    [[nodiscard]] static auto hasRuleSet(const ZoneEntry &entry) -> bool;
    /// Get a zone entry's standard offset in seconds.
    [[nodiscard]] static auto standardOffset(const ZoneEntry &entry) -> int;
    /// Get a zone entry's fixed saving offset in seconds.
    [[nodiscard]] static auto fixedRuleOffset(const ZoneEntry &entry) -> int;
    /// Get a rule's post-transition saving amount in seconds.
    [[nodiscard]] static auto ruleSaveSeconds(const Rule &rule) -> int;
    /// Encode one daylight-saving rule into the runtime word.
    [[nodiscard]] static auto encodedRule(const Rule &rule) -> uint64_t;
    /// Get the UTC boundary and effective offset for a zone continuation.
    [[nodiscard]] auto untilBoundary(const ZoneEntry &entry) const -> std::pair<uint64_t, int>;

private:
    /// Parse the region files and resolve aliases and rule references.
    void parseDatabase(const TimeZoneSource &source);
    /// Parse one tzdb region file.
    void parseRegion(const el::String &region, const el::String &source);
    /// Parse an ordered zone history.
    void addZone(const el::String &region, const el::StringList &lines);
    /// Gather and sort all fragments used by generated names and abbreviations.
    void buildTexts();
    /// Parse and validate CLDR Windows world-territory mappings.
    void parseWindows(const el::String &source);
    /// Walk the CLDR XML tree and gather world-territory mappings.
    void collectWindows(const el::data::xml::XmlNodePtr &node);
    /// Convert one tzdb offset or transition time.
    [[nodiscard]] static auto parseTime(el::String text) -> TimeValue;
    /// Split ASCII whitespace-delimited source fields.
    [[nodiscard]] static auto fields(const el::String &line) -> el::StringList;
    /// Convert a tzdb three-letter month to its number.
    [[nodiscard]] static auto monthNumber(const el::String &month) -> int;
    /// Convert a tzdb weekday to Monday-based index.
    [[nodiscard]] static auto weekdayNumber(const el::String &day) -> int;
    /// Resolve a tzdb day expression for a month.
    [[nodiscard]] static auto resolveDay(int year, int month, const el::String &expression) -> int;
    /// Convert a civil date and relative seconds into the Core epoch count.
    [[nodiscard]] static auto civilSeconds(int year, int month, int day, int seconds) -> int64_t;
    /// Get the last active year of a rule.
    [[nodiscard]] static auto lastYear(const Rule &rule) -> int;
    /// Get the save amount of a rule in minutes.
    [[nodiscard]] static auto saveMinutes(const Rule &rule) -> int;
    /// Determine the most recent rule transitions before a boundary year.
    [[nodiscard]] auto lastTransitions(const RuleSet &set, int year) const -> std::vector<Transition>;
    /// Get one rule's transition in a selected year.
    [[nodiscard]] static auto transition(const Rule &rule, int year) -> Transition;
    /// Parse a zone UNTIL timestamp and its time reference.
    [[nodiscard]] static auto parseUntil(const el::String &text) -> std::pair<int64_t, uint8_t>;
    /// Convert a local timestamp to UTC with the selected reference.
    [[nodiscard]] static auto toUtc(int64_t seconds, uint8_t reference, int standard, int saving) -> int64_t;
    /// Create a valid unique factory identifier from a zone name.
    [[nodiscard]] static auto functionName(const el::String &zone, el::StringSet &used) -> el::String;

private:
    el::StringMap<RuleSetPtr> _ruleSets;                             ///< Parsed named daylight-saving rules.
    el::StringMap<ZonePtr> _zones;                                   ///< Parsed primary zones.
    el::StringMap<el::String> _links;                                ///< Alias-to-primary mapping.
    el::StringList _texts;                                           ///< Sorted compact text fragments.
    std::vector<std::pair<el::String, el::String>> _windowsMappings; ///< CLDR Windows-to-IANA names.
    int _versionYear{};                                              ///< tzdb year.
    int _versionRevision{};                                          ///< tzdb revision letter as 1-based number.
};

}
