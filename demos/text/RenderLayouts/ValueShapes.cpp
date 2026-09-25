// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/unit/ItemIndex.hpp>
#include <erbsland/util/List.hpp>

namespace demo {

/// Build scalar, list, and map values for a terrain report.
///
/// Values preserve their types. A list supplies repeated entries, while a map groups related named fields that a
/// layout can reach through dotted names.
/// @notest{Demo function verified by the documentation executable.}
void valueShapes() {
    using el::render::Value;
    using el::render::ValueList;
    using el::render::ValueMap;

    // Create one nested record and an ordered list of terrain labels.
    auto site = ValueMap{};
    site.set("name"_el, Value{"Valle Clara"_el}).set("height"_el, Value{1920});
    const auto terrain = ValueList{Value{"valle"_el}, Value{"meseta"_el}, Value{"sierra"_el}};
    const auto context = el::render::Context{}
                             .set("site"_el, site)
                             .set("terrain"_el, terrain)
                             .set("surveyed"_el, Value{true})
                             .set("ratio"_el, Value{0.75})
                             .set("note"_el, Value{});

    // The same values can be inspected in C++ before they reach a layout.
    const auto siteValue = context.get("site"_el);
    el::io::printLine("First terrain: "_el, context.get("terrain"_el).get(el::unit::ItemIndex{0U}).asText());
    el::io::printLine("Site height: "_el, siteValue.get("height"_el).asInteger(), " m"_el);

    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "terrain.txt"_el)
        .content()
        .writeTextOrThrow(
            "{{ site.name }} ({{ site.height }} m)\n"
            "{% for label in terrain %}- {{ label }}\n{% endfor %}"
            "Surveyed: {{ surveyed }}; ratio: {{ ratio }}\n"_el);
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(environment->render("terrain.txt"_el, context));
}

}
