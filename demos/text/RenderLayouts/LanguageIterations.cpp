// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/Value.hpp>
#include <erbsland/util/List.hpp>

namespace demo {

/// Render a list and an ordered map with for loops.
///
/// A list loop exposes one item and loop position data. A map loop exposes a key and value. The optional else branch
/// gives a useful result when a collection is empty.
/// @notest{Demo function verified by the documentation executable.}
void languageIterations() {
    using el::render::Value;
    using el::render::ValueList;
    using el::render::ValueMap;

    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "steps.txt"_el)
        .content()
        .writeTextOrThrow(
            "{% for step in steps %}{{ loop.index }}. {{ step }}\n"
            "{% else %}手順なし\n{% endfor %}"_el);
    (directory->path() / "rewards.txt"_el)
        .content()
        .writeTextOrThrow("{% for name, count in rewards %}{{ name }}: {{ count }}\n{% endfor %}"_el);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));

    // List entries appear in their original order; an empty list selects the else branch.
    const auto steps = ValueList{Value{"望遠鏡を準備"_el}, Value{"月を観察"_el}};
    el::io::print(environment->render("steps.txt"_el, el::render::Context{}.set("steps"_el, steps)));
    el::io::print(environment->render("steps.txt"_el, el::render::Context{}.set("steps"_el, ValueList{})));

    // A map supplies two loop targets: its key and its value.
    auto rewards = ValueMap{};
    rewards.set("星図"_el, Value{1}).set("観測記録"_el, Value{2});
    el::io::print(environment->render("rewards.txt"_el, el::render::Context{}.set("rewards"_el, rewards)));
}

}
