// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Work.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionSet.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>

using namespace el::text::literals;

/// Validate parsed configuration before queuing main-loop startup.
auto main(const int argc, char *argv[]) -> int {
    auto app = el::Application{argc, argv};
    auto work = demo::Work{app};
    auto options = el::OptionSet::create();
    options->addOption({"--input"_el, "input"_el})
        .setType(el::OptionType::Text)
        .setRequired()
        .setHelpTitle("Input to process."_el);
    options->setPostParsingFn([&app, &work](const el::OptionValuesPtr &values) -> void {
        const auto input = values->getText("input"_el).trimmed();
        if (input.isEmpty()) {
            throw el::OptionError{"Invalid Input"_el, "Input must not be blank."_el, values->value("input"_el)};
        }
        app.loop()->invoke([&work, input]() -> void { work.begin(input); });
    });
    app.options()->addSet(options);
    return app.run();
}
