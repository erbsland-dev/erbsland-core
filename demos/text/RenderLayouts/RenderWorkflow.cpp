// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/RenderError.hpp>

namespace demo {

/// Load a named layout and render it with a fresh local context.
///
/// Create and configure the environment once. The file-system loader resolves the logical layout name below its
/// root directory. Each render call receives its own context, so the same layout can produce different documents.
void renderWorkflow() {
    const auto temporary = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    const auto layout = temporary->path() / "rhythm.txt"_el;
    layout.content().writeTextOrThrow(
        "Esercizio: {{ title }}\n"
        "Tempo: {{ bpm }} BPM\n"
        "{% if practice %}Ripeti lentamente.\n{% endif %}"_el);

    // A loader maps logical names to files below its absolute root directory.
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(temporary->path()));

    // Prepare a separate local context for this rendering.
    auto context = el::render::Context{};
    context.set("title"_el, "Tre battiti"_el).set("bpm"_el, 72).set("practice"_el, true);
    el::io::print(environment->render("rhythm.txt"_el, context));

    // A missing layout is reported as a render error.
    try {
        el::io::print(environment->render("missing.txt"_el, context));
    } catch (const el::render::RenderError &error) {
        el::io::printLine("Could not render: "_el, error.context().layout());
    }
}

}
