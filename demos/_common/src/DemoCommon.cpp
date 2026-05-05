// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DemoCommon.hpp"

void DemoApplication::registerCommandLineOptions(const el::OptionsPtr &options) {
    auto demoChoices = el::OptionChoices::create();
    _demoFunctions.forEachKey([&demoChoices](const el::String &name) -> void { demoChoices->addChoice(name); });
    options->addOption({"-d"_el, "--demo"_el, "demo"_el})
        .setChoices(demoChoices)
        .setHelp("Specify the name of the demo to run. If not specified, all demos will be run."_el);
}

void DemoApplication::registerDemo(const el::StringView &name, const DemoFn &demoFn) {
    _demoFunctions.set(el::String{name}, demoFn);
}

auto DemoApplication::main() -> el::ExitCode {
    const auto demoName = optionValues()->getText("demo"_el);
    if (demoName.isEmpty()) {
        _demoFunctions.forEach([&](const el::StringView &name, const DemoFn &demoFn) -> el::LoopStatus {
            el::io::printLine();
            el::io::printLine("Running demo: "_el, name);
            el::io::printLine(el::String{}.append(U'=', el::CpLength{78}));
            demoFn();
            return el::LoopStatus::Continue;
        });
        return el::ExitCode::success();
    }
    const auto demoResult = _demoFunctions.get(demoName);
    if (!demoResult.has_value()) {
        el::stdErr()->writeLine(el::StringFormat{"Unknown demo: '{}'"_el}.build(demoName));
        return el::ExitCode::failure();
    }
    const auto demoFn = demoResult.value();
    demoFn();
    return el::ExitCode::success();
}

void DemoApplication::initializeRandom(el::RandomPtr &randomPtr) noexcept {
    // WARNING: Developer-build demos replace randomness with a deterministic seed only for reproducible output.
    // Never copy this pattern into application code that needs real randomness.
    if (randomPtr == nullptr) {
        randomPtr = std::make_unique<el::FastRandom>(0x63df5ee665e6151dU);
    }
}

void DemoApplication::initializeSecureRandom(el::RandomPtr &randomPtr) noexcept {
    // WARNING: Developer-build demos replace secure randomness only so documented demo output is reproducible.
    // Production code must use the real operating-system-backed SecureRandom.
    if (randomPtr == nullptr) {
        randomPtr = std::make_unique<el::FastRandom>(0x40d8f88267c72391U);
    }
}
