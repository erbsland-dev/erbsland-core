// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/SourceResolver.hpp>
#include <erbsland/path/all.hpp>

namespace demo {

/// Resolve an application-owned include scheme to files beneath a controlled directory.
///
/// A custom resolver receives the raw include text and the identifier of the document containing it. It must either
/// return a list of closed sources in parsing order or throw `ConfError`. Returning file sources preserves useful,
/// stable identifiers for include-loop detection and diagnostics.
class PresetSourceResolver final : public el::conf::SourceResolver {
public:
    explicit PresetSourceResolver(el::Path root) : _root{std::move(root)} {}

public: // implement `SourceResolver`
    auto resolve(const el::conf::SourceResolverContext &context) -> el::conf::SourceListPtr override {
        if (context.includeText != "preset:forest"_el) {
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::Syntax, "Unknown application configuration preset."_el};
        }
        auto sources = std::make_shared<el::conf::SourceList>();
        sources->push_back(el::conf::Source::fromFile(_root / "forest.elcl"_el));
        return sources;
    }

private:
    el::Path _root;
};

/// Install a custom source resolver for application-specific include descriptors.
void customResolver() {
    auto temporaryOptions = el::PathTempDirectoryOptions{};
    temporaryOptions.setPrefix("levantamento-"_el).setRandomLength(el::CpLength{8U});
    const auto temporary =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(temporaryOptions);
    const auto root = temporary->path();
    (root / "forest.elcl"_el)
        .content()
        .writeTextOrThrow(
            "[survey]\n"
            "forest: \"Floresta Nacional de Ipanema\"\n"_el);
    const auto mainPath = root / "survey.elcl"_el;
    mainPath.content().writeTextOrThrow("@include: \"preset:forest\"\n"_el);

    auto parser = el::conf::Parser{};
    parser.setSourceResolver(std::make_shared<PresetSourceResolver>(root));
    const auto document = parser.parseFileOrThrow(mainPath);
    el::io::printLine("Forest: "_el, document->getTextOrThrow("survey.forest"_el));
}

}
