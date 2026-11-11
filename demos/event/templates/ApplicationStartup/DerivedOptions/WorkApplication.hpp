// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Application.hpp>
#include <erbsland/EventTarget.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/OptionError.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/OptionType.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/String.hpp>

namespace demo {

using namespace el::text::literals;

/// Own event-driven work while retaining Application's default main loop.
class WorkApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void registerCommandLineOptions(const el::OptionsPtr &options) override {
        options->addOption({"--input"_el, "input"_el})
            .setType(el::OptionType::Text)
            .setRequired()
            .setHelpTitle("Input to process."_el);
    }
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override {
        _input = values->getText("input"_el).trimmed();
        if (_input.isEmpty()) {
            throw el::OptionError{"Invalid Input"_el, "Input must not be blank."_el, values->value("input"_el)};
        }
        loop()->invoke([this]() -> void { beginWork(); });
    }
    void cleanup() noexcept override {
        // TODO: Release application resources; initialization may have failed partway.
        // Main-loop callbacks cannot run here.
    }

private:
    /// Create event sources in the context of the running main loop.
    void beginWork() {
        // TODO: Start and retain your business operations here.
        // Connect their final successful completion to completeWork().
    }
    /// Request shutdown after all required work has finished on this loop.
    void completeWork() { quit(); }

private:
    el::String _input; ///< Validated configuration retained for event callbacks.
};

}
