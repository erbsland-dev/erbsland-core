// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZoneRenderer.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/ItemCount.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSet.hpp>
#include <erbsland/StringTransformations.hpp>

#include <algorithm>
#include <set>

namespace tool::time_zone {

using el::text::render::Context;
using el::text::render::ValueList;
using el::text::render::ValueMap;
using namespace el::text::literals;

auto TimeZoneRenderer::zoneAbbreviations(const TimeZoneModel::Zone &zone) const -> ZoneAbbreviations {
    auto data = ZoneAbbreviations{};
    for (const auto &entry : zone.entries) {
        if (TimeZoneModel::hasRuleSet(entry)) {
            data.usedRuleSets.set(entry.ruleName, _model.ruleSets().get(entry.ruleName).value());
        }
        const auto dynamic =
            TimeZoneModel::hasRuleSet(entry) && (entry.format.contains("%s"_el) || entry.format.contains("%z"_el));
        if (!dynamic) {
            const auto saving = TimeZoneModel::hasRuleSet(entry) ? 0 : TimeZoneModel::fixedRuleOffset(entry);
            const auto text = _model.abbreviation(entry, {}, TimeZoneModel::standardOffset(entry) + saving);
            data.fixedIds.push_back(_model.textId(text));
            data.abbreviationLists.emplace_back();
            continue;
        }
        auto current = std::vector<uint32_t>{};
        auto nonzero = std::set<uint32_t>{};
        const auto set = _model.ruleSets().get(entry.ruleName).value();
        for (const auto &rule : set->rules) {
            const auto text = _model.abbreviation(
                entry, rule.letters, TimeZoneModel::standardOffset(entry) + TimeZoneModel::ruleSaveSeconds(rule));
            const auto id = _model.textId(text);
            current.push_back(id);
            if (id != 0U) {
                nonzero.insert(id);
            }
        }
        if (nonzero.size() == 1U) {
            data.fixedIds.push_back(*nonzero.begin());
            data.abbreviationLists.emplace_back();
        } else {
            data.fixedIds.push_back(0U);
            data.abbreviationLists.push_back(std::move(current));
        }
    }
    packAbbreviations(data);
    return data;
}

void TimeZoneRenderer::packAbbreviations(ZoneAbbreviations &data) {
    for (const auto &list : data.abbreviationLists) {
        if (list.empty()) {
            data.packedOffsets.push_back(0U);
            continue;
        }
        auto found = data.packedIds.size();
        for (auto index = 0U; index < data.packedIds.size(); ++index) {
            const auto overlap = std::min(list.size(), data.packedIds.size() - index);
            if (std::equal(
                    list.begin(),
                    list.begin() + static_cast<std::ptrdiff_t>(overlap),
                    data.packedIds.begin() + static_cast<std::ptrdiff_t>(index))) {
                found = index;
                break;
            }
        }
        for (auto index = data.packedIds.size() - found; index < list.size(); ++index) {
            data.packedIds.push_back(list[index]);
        }
        data.packedOffsets.push_back(static_cast<uint32_t>(found + 1U));
    }
    auto textSet = std::set<uint32_t>{};
    for (const auto id : data.fixedIds) {
        if (id != 0U) {
            textSet.insert(id);
        }
    }
    for (const auto id : data.packedIds) {
        if (id != 0U) {
            textSet.insert(id);
        }
    }
    data.textIds = std::vector<uint32_t>{textSet.begin(), textSet.end()};
}

auto TimeZoneRenderer::zoneContinuations(
    const TimeZoneModel::Zone &zone, const ZoneAbbreviations &data, const el::StringList &ruleNames) const
    -> ValueList {
    auto continuations = ValueList{};
    for (auto index = 0U; index < zone.entries.size(); ++index) {
        const auto &entry = zone.entries[index];
        auto ruleOrOffset = 0;
        if (TimeZoneModel::hasRuleSet(entry)) {
            const auto it = std::find(ruleNames.begin(), ruleNames.end(), entry.ruleName);
            ruleOrOffset =
                0x10000 + static_cast<int>(it - ruleNames.begin()) + static_cast<int>(data.packedOffsets[index] << 4U);
        } else {
            ruleOrOffset = TimeZoneModel::fixedRuleOffset(entry);
        }
        const auto [until, offset] = _model.untilBoundary(entry);
        const auto fixedIt = std::find(data.textIds.begin(), data.textIds.end(), data.fixedIds[index]);
        const auto fixed = data.fixedIds[index] == 0U ? 0U : static_cast<uint32_t>(fixedIt - data.textIds.begin()) + 1U;
        auto line = ValueMap{};
        line.set("standard"_el, TimeZoneModel::standardOffset(entry));
        line.set("until"_el, hex(until, 10U));
        line.set("offset"_el, offset);
        line.set("rule_or_offset"_el, ruleOrOffset);
        line.set("abbreviation"_el, hex(fixed, 2U));
        continuations.append(line);
    }
    return continuations;
}

auto TimeZoneRenderer::zoneContext(const TimeZoneModel::Zone &zone) const -> ValueMap {
    const auto data = zoneAbbreviations(zone);
    if (data.textIds.size() > 31U || data.usedRuleSets.count() > el::ItemCount{16U}) {
        throw el::ApplicationError{
            "Time-Zone Table Exceeds Runtime Limit"_el,
            el::StringFormat{"Zone {} has too many rule or text entries."_el}.build(zone.name)};
    }
    auto ruleNames = el::StringList{};
    auto ruleValues = ValueList{};
    static const auto cRuleSetFormat = el::StringFormat{"ruleSet({}U),"_el};
    for (const auto &[name, set] : data.usedRuleSets) {
        ruleNames.append(name);
        ruleValues.append(cRuleSetFormat.build(set->id));
    }
    const auto continuations = zoneContinuations(zone, data, ruleNames);
    auto textValues = ValueList{};
    for (const auto id : data.textIds) {
        textValues.append(el::join(hex(id, 4U), "U,"_el));
    }
    auto abbreviationValues = ValueList{};
    for (const auto id : data.packedIds) {
        const auto it = std::find(data.textIds.begin(), data.textIds.end(), id);
        const auto mapped = id == 0U ? 0U : static_cast<uint32_t>(it - data.textIds.begin()) + 1U;
        abbreviationValues.append(el::join(hex(mapped, 2U), "U,"_el));
    }
    auto result = ValueMap{};
    result.set("name"_el, zone.name);
    result.set("function"_el, zone.functionName);
    result.set("rule_sets"_el, ruleValues);
    result.set("continuations"_el, continuations);
    result.set("texts"_el, textValues);
    result.set("abbreviations"_el, abbreviationValues);
    return result;
}

void TimeZoneRenderer::renderZones() {
    const auto renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp);
    auto sortedRegions = el::StringSet{};
    for (const auto &region : TimeZoneModel::regions()) {
        if (region == "backward"_el) {
            continue;
        }
        const auto className = el::toAsciiCapitalized(region);
        sortedRegions.insert(className);
        const auto filename = el::join("Region"_el, className);
        auto zones = ValueList{};
        for (const auto &[name, zone] : _model.zones()) {
            if (zone->region == region) {
                zones.append(zoneContext(*zone));
            }
        }
        auto context = Context{};
        context.set("filename"_el, filename);
        context.set("zones"_el, zones);
        add(el::join(filename, ".hpp"_el), renderer->render("time_zone/region_header.cpp"_el, context));
        add(el::join(filename, ".cpp"_el), renderer->render("time_zone/region.cpp"_el, context));
    }
    auto functions = ValueList{};
    for (const auto &[name, zone] : _model.zones()) {
        functions.append(zone->functionName);
    }
    auto context = Context{};
    context.set("regions"_el, sortedRegions);
    context.set("functions"_el, functions);
    context.set("zone_count"_el, _model.zones().count().toSizeT());
    add("Zones.hpp"_el, renderer->render("time_zone/zones_header.cpp"_el));
    add("Zones.cpp"_el, renderer->render("time_zone/zones.cpp"_el, context));
}

}
