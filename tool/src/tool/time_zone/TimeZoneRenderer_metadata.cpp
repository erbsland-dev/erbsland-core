// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TimeZoneRenderer.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/render/Context.hpp>
#include <erbsland/StringFormat.hpp>

namespace tool::time_zone {

using el::text::render::Context;
using el::text::render::ValueList;
using el::text::render::ValueMap;
using namespace el::text::literals;

void TimeZoneRenderer::renderMetadata() {
    const auto renderer = frame::app().createRenderEnvironment(frame::TemplateLanguage::Cpp);
    auto version = Context{};
    version.set("year"_el, _model.versionYear());
    version.set("revision"_el, _model.versionRevision());
    add("Database_version.cpp"_el, renderer->render("time_zone/version.cpp"_el, version));
    add("WindowsTimeZoneMap.hpp"_el, renderer->render("time_zone/windows_map_header.cpp"_el));
    auto mappings = ValueList{};
    for (const auto &[windowsName, ianaName] : _model.windowsMappings()) {
        auto mapping = ValueMap{};
        mapping.set("windows"_el, windowsName.replacedAll("\\"_el, "\\\\"_el).replacedAll("\""_el, "\\\""_el));
        mapping.set("iana"_el, ianaName);
        mapping.set("id"_el, hex(_model.zoneFor(ianaName).id, 4U));
        mappings.append(mapping);
    }
    auto context = Context{};
    context.set("mappings"_el, mappings);
    add("WindowsTimeZoneMap.cpp"_el, renderer->render("time_zone/windows_map.cpp"_el, context));
}

}
