// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/path/all.hpp>

namespace demo {

/// Resolve relative file includes and deterministic wildcard batches with the parser defaults.
///
/// `FileSourceResolver` interprets an include relative to the file that contains it. A filename wildcard can expand to
/// several regular files; the resolver sorts them before the parser processes their contents.
void defaultFileResolver() {
    auto temporaryOptions = el::PathTempDirectoryOptions{};
    temporaryOptions.setPrefix("levantamento-"_el).setRandomLength(el::CpLength{8U});
    const auto temporary =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
    const auto root = temporary->path();
    auto writeOptions = el::PathWriteTextOptions{};
    writeOptions.setCreateParents(true);

    (root / "parts/01-forest.elcl"_el)
        .content()
        .writeTextOrThrow(
            "[survey]\n"
            "forest: \"Parque Nacional da Tijuca\"\n"_el,
            writeOptions);
    (root / "parts/02-team.elcl"_el)
        .content()
        .writeTextOrThrow(
            "[team]\n"
            "observers: 8\n"_el,
            writeOptions);
    const auto mainPath = root / "survey.elcl"_el;
    mainPath.content().writeTextOrThrow("@include: \"parts/*.elcl\"\n"_el);

    // The default resolver expands the wildcard relative to survey.elcl.
    const auto document = el::conf::Parser{}.parseFileOrThrow(mainPath);
    el::io::printLine("Forest: "_el, document->getTextOrThrow("survey.forest"_el));
    el::io::printLine("Observers: "_el, document->getIntegerOrThrow("team.observers"_el));
}

}
