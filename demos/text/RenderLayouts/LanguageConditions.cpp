// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>

#include <initializer_list>

namespace demo {

/// Select one status line from an if, elif, and else chain.
///
/// Conditions inspect context values without requiring the caller to choose or assemble layout fragments.
/// @notest{Demo function verified by the documentation executable.}
void languageConditions() {
    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "status.txt"_el)
        .content()
        .writeTextOrThrow(
            "{% if completed >= required %}達成"
            "{% elif completed > 0 %}進行中"
            "{% else %}未着手{% endif %}\n"_el);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));

    // Reuse the same layout for three states of the quest.
    for (const auto completed : {0, 1, 3}) {
        const auto context = el::render::Context{}.set("completed"_el, completed).set("required"_el, 3);
        el::io::print(environment->render("status.txt"_el, context));
    }
}

}
