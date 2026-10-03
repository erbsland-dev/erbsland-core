// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZoneRenderer.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/render/Context.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/StringJoins.hpp>
#include <erbsland/StringSet.hpp>

namespace tool::time_zone {

using el::text::render::Context;
using el::text::render::Value;
using el::text::render::ValueList;
using namespace el::text::literals;

TimeZoneRenderer::TimeZoneRenderer(const TimeZoneModel &model) : _model{model} {
    renderTexts();
    renderRules();
    renderZones();
    renderMetadata();
}

void TimeZoneRenderer::add(el::String name, const el::String &body) {
    _outputs.emplace_back(std::move(name), body);
}

auto TimeZoneRenderer::hex(const uint64_t value, const uint32_t digits) -> el::String {
    static const auto cHex2 = el::StringFormat{"0x{:number:hex,precision=2,lc=lowercase}"_el};
    static const auto cHex4 = el::StringFormat{"0x{:number:hex,precision=4,lc=lowercase}"_el};
    static const auto cHex10 = el::StringFormat{"0x{:number:hex,precision=10,lc=lowercase}"_el};
    static const auto cHex16 = el::StringFormat{"0x{:number:hex,precision=16,lc=lowercase}"_el};
    if (digits == 2U) {
        return cHex2.build(value);
    }
    if (digits == 4U) {
        return cHex4.build(value);
    }
    if (digits == 10U) {
        return cHex10.build(value);
    }
    return cHex16.build(value);
}

void TimeZoneRenderer::renderTexts() {
    const auto &texts = _model.texts();
    auto block = el::StringEditor{};
    auto entries = ValueList{};
    const auto prepareText = [this](const uint32_t offset, const uint32_t length) {
        return el::join("Text{"_el, hex(offset, 4U), "U, "_el, hex(length, 2U), "U},"_el);
    };
    entries.append(prepareText(0U, 0U));
    for (auto index = 1U; index < texts.count().toSizeT(); ++index) {
        const auto &text = texts.getRef(el::ItemIndex::fromSizeTOrThrow(index));
        const auto stored = el::String{block};
        auto offset = static_cast<uint32_t>(block.length().toSizeT());
        if (const auto match = stored.splitAtFirst(text)) {
            offset = static_cast<uint32_t>(match->first.length().toSizeT());
        } else {
            block.append(text);
        }
        entries.append(prepareText(offset, static_cast<uint32_t>(text.length().toSizeT())));
    }
    if (block.length() > el::ByteCount{0xFFFFU} || texts.count() > el::ItemCount{0xFFFFU}) {
        throw el::ApplicationError{
            "Time-Zone Text Exceeds Runtime Limit"_el, "The generated text block exceeds 16-bit offsets."_el};
    }
    auto allNames = el::StringSet{};
    for (const auto &[name, zone] : _model.zones()) {
        allNames.insert(name);
    }
    for (const auto &[name, target] : _model.links()) {
        allNames.insert(name);
    }
    const auto prepareName = [this](const el::String &name) {
        const auto parts = _model.zoneName(name);
        return el::join(
            "ZoneName{"_el,
            hex(parts[0], 4U),
            "U, "_el,
            hex(parts[1], 4U),
            "U, "_el,
            hex(parts[2], 4U),
            "U, "_el,
            hex(parts[3], 4U),
            "U},"_el);
    };
    auto names = ValueList{};
    auto primary = ValueList{};
    for (const auto &name : allNames) {
        names.append(prepareName(name));
    }
    for (const auto &[name, zone] : _model.zones()) {
        primary.append(prepareName(name));
    }
    const auto renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp);
    auto context = Context{};
    context.set("entry_count"_el, texts.count().toSizeT());
    context.set("name_count"_el, allNames.count().toSizeT());
    context.set("primary_count"_el, _model.zones().count().toSizeT());
    context.set("entries"_el, entries);
    context.set("names"_el, names);
    context.set("primary"_el, primary);
    context.set("block"_el, el::String{block}.replacedAll("\\"_el, "\\\\"_el).replacedAll("\""_el, "\\\""_el));
    add("Texts.hpp"_el, renderer->render("time_zone/texts_header.cpp"_el, context));
    add("Texts.cpp"_el, renderer->render("time_zone/texts.cpp"_el, context));
}

void TimeZoneRenderer::renderRules() {
    const auto renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp);
    add("Rules.hpp"_el, renderer->render("time_zone/rules_header.cpp"_el));
    auto rules = ValueList{};
    auto offsets = ValueList{};
    const auto offsetFormat = el::StringFormat{"{}U,"_el};
    auto count = 0U;
    for (const auto &[name, set] : _model.ruleSets()) {
        offsets.append(offsetFormat.build(count));
        for (const auto &rule : set->rules) {
            rules.append(el::join("Rule{"_el, hex(TimeZoneModel::encodedRule(rule), 16U), "ULL},"_el));
            ++count;
        }
    }
    offsets.append(offsetFormat.build(count));
    rules.append(el::join("Rule{"_el, hex(0U, 16U), "ULL},"_el));
    auto context = Context{};
    context.set("rules"_el, rules);
    context.set("rule_count"_el, count + 1U);
    context.set("offsets"_el, offsets);
    context.set("offset_count"_el, _model.ruleSets().count().toSizeT() + 1U);
    add("Rules.cpp"_el, renderer->render("time_zone/rules.cpp"_el, context));
}

}
