// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/text/render/Context.hpp>
#include <erbsland/text/render/Environment.hpp>
#include <erbsland/text/render/FileSystemLoader.hpp>
#include <erbsland/text/render/Value.hpp>
#include <erbsland/util/List.hpp>

namespace demo {

/// Combine named values, expressions, and filters in a quest note.
///
/// Dotted lookup reads a member from a map. An expression can calculate a value before insertion, and a filter can
/// transform the result. Missing names resolve to null, which the default filter can replace with useful text.
/// @notest{Demo function verified by the documentation executable.}
void languageExpressions() {
    using el::render::Value;
    using el::render::ValueList;
    using el::render::ValueMap;

    const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    (directory->path() / "quest.txt"_el)
        .content()
        .writeTextOrThrow(
            "依頼: {{ quest.title | trim }}\n"
            "次の段階: {{ completed + 1 }} / {{ steps | length }}\n"
            "手順: {{ steps | join(' → ') }}\n"
            "担当: {{ owner | default('未定') }}\n"_el);

    // The layout reads a nested map, a list, and an integer from one local context.
    auto quest = ValueMap{};
    quest.set("title"_el, Value{"  星空を調べる  "_el});
    const auto steps = ValueList{Value{"望遠鏡を準備"_el}, Value{"月を観察"_el}, Value{"星図を記録"_el}};
    const auto context = el::render::Context{}.set("quest"_el, quest).set("steps"_el, steps).set("completed"_el, 1);

    const auto environment = el::render::Environment::create();
    environment->addLayoutLoader(el::render::FileSystemLoader::create(directory->path()));
    el::io::print(environment->render("quest.txt"_el, context));
}

}
