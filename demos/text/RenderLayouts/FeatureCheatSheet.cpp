// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/Value.hpp>
#include <erbsland/util/List.hpp>

namespace demo {

/// Render a bird nesting record from a compact layout.
///
/// A map supplies named fields, a list supplies observations, and the layout combines expressions, tests, filters,
/// conditions, local assignments, and iteration without changing the caller's context.
/// @notest{Demo function verified by the documentation executable.}
void featureCheatSheet() {
    using el::render::Value;
    using el::render::ValueList;
    using el::render::ValueMap;

    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "nest.txt"_el)
        .content()
        .writeTextOrThrow(
            "Pesä: {{ nest.species | capitalize }}\n"
            "{% set next_count = nest.eggs + 1 %}Munia: {{ nest.eggs }}; seuraava: {{ next_count }}\n"
            "{% if nest.eggs is odd %}Pariton määrä\n{% endif %}"
            "{% for sighting in sightings %}{{ loop.index }}. {{ sighting | trim }}\n{% endfor %}"
            "Merkitsijä: {{ keeper | default('tuntematon') }}\n"_el);

    // Prepare the record and its observations for one render call.
    auto nest = ValueMap{};
    nest.set("species"_el, Value{"kirjosieppo"_el}).set("eggs"_el, Value{3});
    const auto sightings = ValueList{Value{"  aamulla  "_el}, Value{"illalla"_el}};
    const auto context = el::render::Context{}.set("nest"_el, nest).set("sightings"_el, sightings);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(environment->render("nest.txt"_el, context));
}

}
