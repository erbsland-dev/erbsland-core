// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "SecurityHashesTool.hpp"

#include "Inventory.hpp"
#include "Manifest.hpp"

#include "../frame/ToolApplication.hpp"

#include <erbsland/OptionError.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/system/SystemInfo.hpp>

#include <algorithm>

namespace tool::security_hashes {

using namespace el::text::literals;

auto SecurityHashesTool::identifier() const -> el::String {
    return "security_hashes"_el;
}

auto SecurityHashesTool::title() const -> el::String {
    return "Generate security hashes"_el;
}

auto SecurityHashesTool::description() const -> el::String {
    return "Hash selected source files in parallel, normalizing text line endings for portable checkout verification."_el;
}

auto SecurityHashesTool::optionModule() const -> el::OptionModulePtr {
    auto module = Tool::optionModule();
    module->addOption({"--verify"_el, "--check"_el, cVerifyOption})
        .setHelpTitle("Verify the security hash manifest"_el)
        .setHelpDescription("Fail if the manifest is missing or outdated, without writing any files."_el);
    module->addOption({"--jobs"_el, "-j"_el, cJobsOption})
        .setType(el::OptionType::Integer)
        .setValueName("count"_el)
        .setHelpTitle("Parallel hashing workers"_el)
        .setHelpDescription(
            "Use 1 to 64 workers; the default is the logical CPU count, limited to 64 and the number of selected files."_el);
    return module;
}

auto SecurityHashesTool::configurationRules() const -> el::conf::vr::RulesPtr {
    return Settings::rules();
}

void SecurityHashesTool::configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) {
    _workers = std::clamp(el::system::info::logicalCpuCount(), 1U, 64U);
    if (const auto option = values->value(cJobsOption)) {
        const auto count = values->getInteger(cJobsOption);
        if (count < 1 || count > 64) {
            throw el::OptionError{"Invalid Worker Count"_el, "--jobs must be between 1 and 64."_el, option};
        }
        _workers = static_cast<unsigned int>(count);
    }
    _verify = values->getFlag(cVerifyOption);
    _settings.read(configuration, frame::app().files());
}

void SecurityHashesTool::execute() {
    const auto &files = frame::app().files();
    taskContext().progress.beginPhase("Discovering hash inputs"_el);
    const auto inputs = Inventory{_settings, files}.collect();
    log()->info("Hashing "_el, inputs.count(), " source files..."_el);
    const auto manifest = Manifest{files};
    const auto text = manifest.generate(inputs, _workers, taskContext().progress);
    if (manifest.publish(_settings.output, text, _verify)) {
        log()->info("Updated "_el, _settings.output);
    } else {
        log()->info("Verified "_el, _settings.output);
    }
}

}
