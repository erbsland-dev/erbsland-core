// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PreprocessSourcesTool.hpp"

#include "impl/SourcePreprocessor.hpp"

#include <tool/frame/ToolApplication.hpp>

namespace tool::doc {

using namespace el::text::literals;

auto PreprocessSourcesTool::identifier() const -> el::String {
    return "doc_preprocess_sources"_el;
}

auto PreprocessSourcesTool::title() const -> el::String {
    return "Prepare cached Doxygen input."_el;
}

auto PreprocessSourcesTool::description() const -> el::String {
    return "Transform source headers for Doxygen, preserving template constraints and reusing unchanged output."_el;
}

auto PreprocessSourcesTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"--force"_el, "force"_el}).setHelpTitle("Regenerate the complete source input tree."_el);
    return module;
}

void PreprocessSourcesTool::configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &values) {
    _force = values->getFlag("force"_el);
}

void PreprocessSourcesTool::execute() {
    auto processor = impl::SourcePreprocessor{frame::app().files()};
    if (processor.run(_force)) {
        log()->info("Doxygen input regenerated."_el);
    } else {
        log()->info("Doxygen input is up to date."_el);
    }
}

}
