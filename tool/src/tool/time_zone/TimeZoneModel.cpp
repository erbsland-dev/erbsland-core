// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZoneModel.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/ItemRange.hpp>
#include <erbsland/StringEditor.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSplitter.hpp>
#include <erbsland/U8StringConstIterator.hpp>
#include <erbsland/xml/XmlDocument.hpp>

#include <algorithm>

namespace tool::time_zone {

using namespace el::text::literals;

TimeZoneModel::TimeZoneModel(const TimeZoneSource &source) {
    parseDatabase(source);
    buildTexts();
    parseWindows(source.text("windowsZones.xml"_el));
}

auto TimeZoneModel::regions() -> el::StringList {
    return el::StringList{
        "africa"_el,
        "antarctica"_el,
        "asia"_el,
        "australasia"_el,
        "europe"_el,
        "northamerica"_el,
        "southamerica"_el,
        "etcetera"_el,
        "backward"_el};
}

auto TimeZoneModel::fields(const el::String &line) -> el::StringList {
    static const auto cFieldWhitespace = el::CharSet{U' ', U'\t', U'\r'};
    return el::StringList::fromSplit(line, cFieldWhitespace);
}

void TimeZoneModel::parseDatabase(const TimeZoneSource &source) {
    const auto version = source.version();
    if (version.characterLength() != el::CpCount{5}) {
        throw el::ApplicationError{"Invalid tzdb Version"_el, "Expected a four-digit year and lowercase revision."_el};
    }
    const auto revision = version.charAt(el::StringSide::Back);
    if (revision < U'a' || revision > U'z') {
        throw el::ApplicationError{"Invalid tzdb Version"_el, "Expected a four-digit year and lowercase revision."_el};
    }
    _versionYear = version.slice(el::StringSide::Front, el::CpCount{4}).toIntegerOrThrow<int>();
    _versionRevision = static_cast<int>(revision.toRawValue() - U'a') + 1;
    for (const auto &region : regions()) {
        parseRegion(region, source.text(el::join("tzdb/"_el, region)));
    }
    auto id = 1U;
    auto usedNames = el::StringSet{};
    for (const auto &[name, zone] : _zones) {
        zone->id = id++;
        zone->functionName = functionName(name, usedNames);
        for (const auto &entry : zone->entries) {
            if (hasRuleSet(entry) && !_ruleSets.contains(entry.ruleName)) {
                throw el::ApplicationError{
                    "Invalid tzdb Zone"_el,
                    el::StringFormat{"Zone {} references unknown rule set {}."_el}.build(name, entry.ruleName)};
            }
        }
    }
    id = 0U;
    for (const auto &[name, set] : _ruleSets) {
        set->id = id++;
    }
    for (const auto &[alias, target] : _links) {
        if (!_zones.contains(target) || _zones.contains(alias)) {
            throw el::ApplicationError{
                "Invalid tzdb Link"_el, el::StringFormat{"Invalid alias {} -> {}."_el}.build(alias, target)};
        }
    }
    if (_zones.count() >= el::ItemCount{0xFFFFU} || _ruleSets.count() >= el::ItemCount{0xFFFFU}) {
        throw el::ApplicationError{
            "Time-Zone Table Exceeds Runtime Limit"_el,
            "The number of zones or rule sets exceeds the 16-bit runtime index."_el};
    }
}

void TimeZoneModel::parseRegion(const el::String &region, const el::String &source) {
    auto zoneLines = el::StringList{};
    auto lines = el::StringSplitter{source, U'\n'};
    while (!lines.isAtEnd()) {
        auto line = lines.next();
        if (const auto comment = line.splitAtFirst("#"_el)) {
            line = comment->first;
        }
        line = line.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const auto tokens = fields(line);
        if (tokens.first() == "Rule"_el) {
            if (tokens.count() < el::ItemCount{10}) {
                throw el::ApplicationError{"Invalid tzdb Rule"_el, line};
            }
            const auto &name = tokens.getRef(el::ItemIndex{1});
            auto set = _ruleSets.get(name, nullptr);
            if (!set) {
                set = std::make_shared<RuleSet>();
                set->name = name;
                _ruleSets.set(name, set);
            }
            set->rules.push_back(
                Rule{
                    name,
                    tokens.getRef(el::ItemIndex{2}).toIntegerOrThrow<int>(),
                    tokens.get(el::ItemIndex{3}),
                    tokens.get(el::ItemIndex{5}),
                    tokens.get(el::ItemIndex{6}),
                    tokens.get(el::ItemIndex{7}),
                    tokens.get(el::ItemIndex{8}),
                    tokens.get(el::ItemIndex{9})});
        } else if (tokens.first() == "Link"_el) {
            if (tokens.count() < el::ItemCount{3}) {
                throw el::ApplicationError{"Invalid tzdb Link"_el, line};
            }
            _links.set(tokens.getRef(el::ItemIndex{2}), tokens.get(el::ItemIndex{1}));
        } else if (tokens.first() == "Zone"_el) {
            if (!zoneLines.isEmpty()) {
                addZone(region, zoneLines);
                zoneLines.clear();
            }
            zoneLines.append(line);
        } else if (!zoneLines.isEmpty()) {
            zoneLines.append(line);
        } else {
            throw el::ApplicationError{"Invalid tzdb Record"_el, line};
        }
    }
    if (!zoneLines.isEmpty()) {
        addZone(region, zoneLines);
    }
}

void TimeZoneModel::addZone(const el::String &region, const el::StringList &lines) {
    const auto first = fields(lines.first());
    if (first.count() < el::ItemCount{5}) {
        throw el::ApplicationError{"Invalid tzdb Zone"_el, lines.first()};
    }
    const auto &name = first.getRef(el::ItemIndex{1});
    auto zone = _zones.get(name, nullptr);
    if (!zone) {
        zone = std::make_shared<Zone>();
        zone->name = name;
        zone->region = region;
        _zones.set(name, zone);
    } else if (zone->region != region) {
        throw el::ApplicationError{"Duplicate tzdb Zone"_el, name};
    }
    for (const auto &line : lines) {
        auto tokens = fields(line);
        if (tokens.first() == "Zone"_el) {
            tokens = tokens.slice(el::ItemRange{el::ItemIndex{2}, el::ItemCount::infinite()});
        }
        if (tokens.count() < el::ItemCount{3}) {
            throw el::ApplicationError{"Invalid tzdb Zone"_el, line};
        }
        const auto until = tokens.slice(el::ItemRange{el::ItemIndex{3}, el::ItemCount::infinite()}).join(" "_el);
        zone->entries.push_back(
            ZoneEntry{tokens.get(el::ItemIndex{0}), tokens.get(el::ItemIndex{1}), tokens.get(el::ItemIndex{2}), until});
    }
}

auto TimeZoneModel::hasRuleSet(const ZoneEntry &entry) -> bool {
    if (entry.ruleName == "-"_el) {
        return false;
    }
    const auto first = entry.ruleName[el::CpIndex{0}];
    return !(first.isAsciiDigit() || first == U'+' || first == U'-');
}

auto TimeZoneModel::parseTime(el::String text) -> TimeValue {
    if (text.isEmpty()) {
        throw el::ApplicationError{"Invalid tzdb Time"_el, "A time value is empty."_el};
    }
    auto result = TimeValue{};
    const auto [last, withoutLast] = text.slice(el::StringSide::Back);
    if (last == U'u' || last == U'g' || last == U'z') {
        result.reference = 0U;
    } else if (last == U's') {
        result.reference = 2U;
    }
    if (last == U'u' || last == U'g' || last == U'z' || last == U's' || last == U'w') {
        text = withoutLast;
    }
    auto sign = 1;
    if (text.startsWith("-"_el)) {
        sign = -1;
        text = text.slice(el::StringSide::Back, el::CpIndex{1});
    } else if (text.startsWith("+"_el)) {
        text = text.slice(el::StringSide::Back, el::CpIndex{1});
    }
    auto parts = fields(text.replacedAll(el::CharSet{U':'}, " "_el));
    if (parts.count() > el::ItemCount{3}) {
        throw el::ApplicationError{"Invalid tzdb Time"_el, text};
    }
    auto multipliers = std::array{3600, 60, 1};
    for (auto index = 0U; index < parts.count().toSizeT(); ++index) {
        result.seconds +=
            parts.getRef(el::ItemIndex::fromSizeTOrThrow(index)).toIntegerOrThrow<int>() * multipliers[index];
    }
    result.seconds *= sign;
    return result;
}

auto TimeZoneModel::standardOffset(const ZoneEntry &entry) -> int {
    return parseTime(entry.standardOffset).seconds;
}

auto TimeZoneModel::fixedRuleOffset(const ZoneEntry &entry) -> int {
    return entry.ruleName == "-"_el ? 0 : parseTime(entry.ruleName).seconds;
}

auto TimeZoneModel::zoneFor(const el::String &name) const -> const Zone & {
    const auto target = _links.get(name, name);
    if (const auto zone = _zones.get(target, nullptr)) {
        return *zone;
    }
    throw el::ApplicationError{"Unknown tzdb Zone"_el, name};
}

auto TimeZoneModel::textId(const el::String &text) const -> uint32_t {
    if (text.isEmpty()) {
        return 0U;
    }
    const auto it = std::lower_bound(_texts.begin(), _texts.end(), text);
    if (it == _texts.end() || *it != text) {
        throw el::ApplicationError{"Unknown Time-Zone Text"_el, text};
    }
    return static_cast<uint32_t>(it - _texts.begin());
}

auto TimeZoneModel::zoneName(const el::String &name) const -> std::array<uint32_t, 4> {
    auto result = std::array<uint32_t, 4>{};
    auto parts = el::StringSplitter{name, U'/'};
    auto index = 0U;
    while (!parts.isAtEnd()) {
        if (index >= 3U) {
            throw el::ApplicationError{"Invalid tzdb Zone Name"_el, name};
        }
        result[index++] = textId(parts.next());
    }
    result[3] = zoneFor(name).id;
    return result;
}

auto TimeZoneModel::abbreviation(const ZoneEntry &entry, const el::String &letters, const int offsetSeconds) const
    -> el::String {
    auto result = entry.format;
    if (result.isEmpty() || result == "-"_el || result.startsWith("-"_el) || result.startsWith("+"_el)) {
        return {};
    }
    const auto numeric = result.contains("%z"_el);
    const auto actualLetters = letters == "-"_el ? el::String{} : letters;
    result = result.replacedAll("%s"_el, actualLetters);
    if (numeric) {
        auto absolute = offsetSeconds < 0 ? -offsetSeconds : offsetSeconds;
        auto formatted = el::StringEditor{offsetSeconds < 0 ? "-"_el : "+"_el};
        static const auto pairFormat = el::StringFormat{"{}"_el};
        auto appendPair = [&formatted](const int value) {
            if (value < 10) {
                formatted.append("0"_el);
            }
            formatted.append(pairFormat.build(value));
        };
        appendPair(absolute / 3600);
        if (absolute % 3600 != 0) {
            appendPair((absolute / 60) % 60);
        }
        if (absolute % 60 != 0) {
            appendPair(absolute % 60);
        }
        result = result.replacedAll("%z"_el, formatted);
    }
    if ((result.startsWith("-"_el) || result.startsWith("+"_el)) && !numeric) {
        return {};
    }
    return result;
}

auto TimeZoneModel::abbreviations(const Zone &zone) const -> el::StringSet {
    auto result = el::StringSet{};
    for (const auto &entry : zone.entries) {
        if (entry.format.isEmpty() || entry.format == "-"_el || entry.format.startsWith("-"_el) ||
            entry.format.startsWith("+"_el)) {
            continue;
        }
        if (!entry.format.contains("%s"_el) && !entry.format.contains("%z"_el)) {
            result.insert(entry.format);
            continue;
        }
        if (!hasRuleSet(entry)) {
            const auto text = abbreviation(entry, {}, standardOffset(entry) + fixedRuleOffset(entry));
            if (!text.isEmpty()) {
                result.insert(text);
            }
            continue;
        }
        const auto set = _ruleSets.get(entry.ruleName).value();
        for (const auto &rule : set->rules) {
            const auto text = abbreviation(entry, rule.letters, standardOffset(entry) + saveMinutes(rule) * 60);
            if (!text.isEmpty()) {
                result.insert(text);
            }
        }
    }
    return result;
}

void TimeZoneModel::buildTexts() {
    auto collected = el::StringSet{};
    for (const auto &[name, zone] : _zones) {
        auto parts = el::StringSplitter{name, U'/'};
        auto count = 0U;
        while (!parts.isAtEnd()) {
            collected.insert(parts.next());
            ++count;
        }
        if (count > 3U) {
            throw el::ApplicationError{"Invalid tzdb Zone Name"_el, name};
        }
        const auto zoneAbbreviations = abbreviations(*zone);
        for (const auto &abbreviation : zoneAbbreviations) {
            collected.insert(abbreviation);
        }
    }
    for (const auto &[name, target] : _links) {
        auto parts = el::StringSplitter{name, U'/'};
        auto count = 0U;
        while (!parts.isAtEnd()) {
            collected.insert(parts.next());
            ++count;
        }
        if (count > 3U) {
            throw el::ApplicationError{"Invalid tzdb Alias Name"_el, name};
        }
    }
    _texts.append(el::String{});
    for (const auto &text : collected) {
        if (text.characterLength() > el::CpCount{15}) {
            throw el::ApplicationError{"Time-Zone Text Exceeds Runtime Limit"_el, text};
        }
        if (!text.isEmpty()) {
            _texts.append(text);
        }
    }
    if (_texts.count() > el::ItemCount{0xFFFFU}) {
        throw el::ApplicationError{
            "Time-Zone Text Exceeds Runtime Limit"_el,
            "The number of text entries exceeds the 16-bit runtime index."_el};
    }
}

void TimeZoneModel::parseWindows(const el::String &source) {
    const auto document = el::data::xml::XmlDocument::fromStringOrThrow(source);
    if (!document.root()) {
        throw el::ApplicationError{"Invalid CLDR Windows Zones"_el, "The XML document has no root element."_el};
    }
    collectWindows(document.root());
    std::sort(_windowsMappings.begin(), _windowsMappings.end());
    if (_windowsMappings.empty()) {
        throw el::ApplicationError{"Invalid CLDR Windows Zones"_el, "No world-territory mappings were found."_el};
    }
}

void TimeZoneModel::collectWindows(const el::data::xml::XmlNodePtr &node) {
    if (node->name() == "mapZone"_el && node->getAttribute("territory"_el) == "001"_el) {
        const auto windows = node->getAttribute("other"_el);
        const auto iana = node->getAttribute("type"_el);
        if (!windows || !iana) {
            throw el::ApplicationError{"Invalid CLDR Windows Zones"_el, "A world mapping lacks a name."_el};
        }
        const auto names = fields(*iana);
        if (names.isEmpty()) {
            throw el::ApplicationError{"Invalid CLDR Windows Zones"_el, "A world mapping has no IANA zone."_el};
        }
        (void)zoneFor(names.first());
        _windowsMappings.emplace_back(*windows, names.first());
    }
    for (const auto &child : node->children()) {
        collectWindows(child);
    }
}

auto TimeZoneModel::functionName(const el::String &zone, el::StringSet &used) -> el::String {
    auto name = el::StringEditor{"info"_el};
    for (const auto character : zone) {
        if (character.isAsciiAlphanumeric()) {
            name.append(character);
        } else if (character == U'+') {
            name.append("Plus"_el);
        } else if (character == U'/' || character == U'-') {
            name.append("_"_el);
        }
    }
    const auto base = el::String{name};
    auto result = base;
    auto suffix = 2U;
    const auto suffixFormat = el::StringFormat{"{}_{}"_el};
    while (used.contains(result)) {
        result = suffixFormat.build(base, suffix++);
    }
    used.insert(result);
    return result;
}

}
