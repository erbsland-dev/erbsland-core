// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApiTestStatusTool.hpp"

#include "impl/ApiTestStatus.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/StringJoins.hpp>
#include <tool/frame/ToolApplication.hpp>

namespace tool::doc {

using namespace el::text::literals;

auto ApiTestStatusTool::identifier() const -> el::String {
    return "doc_api_test_status"_el;
}

auto ApiTestStatusTool::title() const -> el::String {
    return "Validate API test status"_el;
}

auto ApiTestStatusTool::description() const -> el::String {
    return "Validate API test-status markers. Reports source locations for invalid constructs."_el;
}

auto ApiTestStatusTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"--normalize"_el, "normalize"_el}).setHelpTitle("Normalize marker syntax before validating."_el);
    return module;
}

void ApiTestStatusTool::configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &values) {
    _normalize = values->getFlag("normalize"_el);
}

void ApiTestStatusTool::execute() {
    auto &files = frame::app().files();
    auto diagnostics = el::StringList{};
    const auto &progress = taskContext().progress;
    progress.beginPhase("Discovering validation inputs"_el);
    const auto paths = files.collectFilenamesOnly(el::Path{"src/erbsland"_el}, ".hpp"_el);
    progress.beginPhase("Validating files"_el, paths.count());
    for (const auto &path : paths) {
        auto text = files.readWalkedTextFile(path).text();
        if (_normalize) {
            text = impl::test_status::normalize(text);
            files.updateText(path, text);
        }
        const auto issues = impl::test_status::validate(text);
        for (const auto &issue : issues) {
            diagnostics.append(el::join(path.toRelative(files.projectRoot()).toString(), ":"_el, issue));
        }
        progress.advance();
    }
    if (!diagnostics.isEmpty()) {
        throw el::ApplicationError{"API Test Status Validation Failed"_el, diagnostics.join("\n"_el)};
    }
}

}
