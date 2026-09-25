// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>

namespace demo {

/// Render a short practice note from a layout and a context.
///
/// A layout holds the stable wording and template syntax. The context supplies values that change for each
/// rendering. A file-system loader gives the environment a named source to render.
void introduction() {
    const auto temporary = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    const auto layout = temporary->path() / "practice.txt"_el;
    layout.content().writeTextOrThrow(
        "{# A note for the practice leader. #}"
        "Esercizio: {{ pattern }}\n"
        "{% if repeat %}Ripeti {{ count }} volte.{% else %}Una volta.{% endif %}\n"_el);

    // Register the directory before rendering any named layout.
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(temporary->path()));

    // Render the same layout with two different local contexts.
    auto context = el::render::Context{};
    context.set("pattern"_el, "ta-ta-TUM"_el).set("repeat"_el, true).set("count"_el, 4);
    el::io::print(environment->render("practice.txt"_el, context));
    context.set("pattern"_el, "TUM-ta"_el).set("repeat"_el, false);
    el::io::print(environment->render("practice.txt"_el, context));
}

}
