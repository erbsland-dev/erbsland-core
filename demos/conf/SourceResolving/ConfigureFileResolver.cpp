// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/FileSourceResolver.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/path/all.hpp>

namespace demo {

/// Restrict file-include syntax before installing a `FileSourceResolver` on a parser.
///
/// Every optional path feature is enabled by default. Disable forms that an application does not need, then pass the
/// configured shared resolver to `Parser::setSourceResolver()`. Exact relative paths continue to work with all optional
/// features disabled.
void configureFileResolver() {
    auto temporaryOptions = el::PathTempDirectoryOptions{};
    temporaryOptions.setPrefix("levantamento-"_el).setRandomLength(el::CpLength{8U});
    const auto temporary =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
    const auto root = temporary->path();
    (root / "defaults.elcl"_el)
        .content()
        .writeTextOrThrow(
            "[survey]\n"
            "region: \"Mata Atlântica\"\n"_el);
    const auto mainPath = root / "survey.elcl"_el;
    mainPath.content().writeTextOrThrow("@include: \"defaults.elcl\"\n"_el);

    // Accept only exact relative paths without protocol prefixes or wildcards.
    const auto resolver = el::conf::FileSourceResolver::create();
    resolver->disable(el::conf::FileSourceResolver::RecursiveWildcard);
    resolver->disable(el::conf::FileSourceResolver::FilenameWildcard);
    resolver->disable(el::conf::FileSourceResolver::AbsolutePaths);
    resolver->disable(el::conf::FileSourceResolver::WindowsUNCPath);
    resolver->disable(el::conf::FileSourceResolver::FileProtocol);

    auto parser = el::conf::Parser{};
    parser.setSourceResolver(resolver);
    const auto document = parser.parseFileOrThrow(mainPath);
    el::io::printLine("Region: "_el, document->getTextOrThrow("survey.region"_el));
}

}
