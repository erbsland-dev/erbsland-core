// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tool/frame/Tool.hpp>

#include <functional>

using namespace el::text::literals;

/// Execute test assertions inside the application's configured command lifecycle.
/// @notest{Unit-test fixture.}
class NoConfigurationTool : public tool::frame::Tool {
public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override { return "probe"_el; }
    [[nodiscard]] auto title() const -> el::String override { return "Run a test probe"_el; }
    [[nodiscard]] auto description() const -> el::String override {
        return "Run assertions with an active application."_el;
    }
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &) override {
        _configuration = configuration;
    }
    void execute() override {
        if (action) {
            action(_configuration);
        }
    }

public: // fixture state
    /// Set only for the lifetime of ProjectFixture::runTool(), which restores the previous callback.
    inline static std::function<void(const el::conf::ValuePtr &)> action;

private:
    el::conf::ValuePtr _configuration; ///< Section passed by the application.
};
