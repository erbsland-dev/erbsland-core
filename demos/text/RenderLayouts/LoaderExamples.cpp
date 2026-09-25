// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/Loader.hpp>
#include <erbsland/text/render/ResourceLoader.hpp>

#include <memory>
#include <optional>

namespace demo {

/// Load named layouts from a directory that an application can edit.
///
/// A file-system loader maps a logical name to a relative file below an existing absolute directory. Several
/// directories can be searched in order, allowing a local layout to take precedence over a bundled one.
/// @notest{Demo function verified by the documentation executable.}
void fileLoader() {
    const auto first = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    const auto second = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (first->path() / "lesson.txt"_el).content().writeTextOrThrow("İlk alıştırma: {{ pattern }}\n"_el);
    (second->path() / "lesson.txt"_el).content().writeTextOrThrow("İkinci alıştırma: {{ pattern }}\n"_el);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(
        el::render::FileSystemLoader::create(el::util::List<el::Path>{first->path(), second->path()}));
    el::io::print(environment->render("lesson.txt"_el, el::render::Context{}.set("pattern"_el, "yavaş-hızlı"_el)));
}

/// Find a layout embedded in the application through its resource identifier.
///
/// The optional prefix places the logical layout tree inside the compiled resource set. The layout name
/// remains the same name used by render, include, and inheritance.
/// @notest{Demo function verified by the documentation executable.}
void resourceLoader() {
    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::ResourceLoader::create("render-layouts"_el, "tempo"_el));
    el::io::print(environment->render(
        "note.txt"_el, el::render::Context{}.set("pattern"_el, "yavaş-hızlı"_el).set("bpm"_el, 84)));
}

/// Prefer an editable file while retaining compiled layouts as defaults.
///
/// The environment tries a higher-priority file loader first. A missing local layout falls through to the
/// resource loader, while a local layout replaces its compiled counterpart.
/// @notest{Demo function verified by the documentation executable.}
void layeredLoaders() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "note.txt"_el).content().writeTextOrThrow("Yerel: {{ pattern }}\n"_el);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::ResourceLoader::create("render-layouts"_el, "tempo"_el));
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()), 10);
    const auto context = el::render::Context{}.set("pattern"_el, "yavaş-hızlı"_el);
    el::io::print(environment->render("note.txt"_el, context));
    el::io::print(environment->render("default.txt"_el, context));
}

}
