// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApiGuidelinesTool.hpp"

#include "impl/ApiGuidelines.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::doc {

using namespace el::text::literals;

auto ApiGuidelinesTool::identifier() const -> el::String {
    return "doc_api_guidelines"_el;
}

auto ApiGuidelinesTool::title() const -> el::String {
    return "Validate API guidelines"_el;
}

auto ApiGuidelinesTool::description() const -> el::String {
    return "Validate API guideline pages. Reports source locations for invalid constructs."_el;
}

void ApiGuidelinesTool::execute() {
    auto &files = frame::app().files();
    auto diagnostics = el::StringList{};
    const auto &progress = taskContext().progress;
    progress.beginPhase("Discovering validation inputs"_el);
    const auto paths = files.collectFilenamesOnly(el::Path{"doc/guidelines/api"_el}, ".rst"_el);
    progress.beginPhase("Validating files"_el, paths.count());
    for (const auto &path : paths) {
        if (path.name() == "common.rst"_el) {
            progress.advance();
            continue;
        }
        const auto text = files.readWalkedTextFile(path).text();
        const auto issues = impl::ApiGuidelines{}.validate(text);
        for (const auto &issue : issues) {
            diagnostics.append(el::join(path.toRelative(files.projectRoot()).toString(), ":"_el, issue));
        }
        progress.advance();
    }
    if (!diagnostics.isEmpty()) {
        throw el::ApplicationError{"API Guideline Validation Failed"_el, diagnostics.join("\n"_el)};
    }
}

}
