// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/Tool.hpp"

#include <erbsland/Literals.hpp>

namespace tool::doc {

using namespace el::text::literals;

/// Create a reviewable multi-part demo using compiled resource templates.
class CreateDemoTool final : public frame::Tool {
    /// Positional option key selecting the domain, demo collection, and part to create.
    static constexpr auto cDemoPath = "demo-path"_el;
    /// Option key for --yes, creating planned demo files without interactive confirmation.
    static constexpr auto cYes = "yes"_el;

public:
    // defaults
    CreateDemoTool() = default;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    void configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &) override;
    void execute() override;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    /// Obtain interactive confirmation after displaying the complete plan.
    auto confirm() const -> bool;

private:
    el::OptionValuePtr _pathOption; ///< Parsed demo path for option diagnostics.
    bool _yes{false};               ///< Explicitly skip interactive confirmation.
};

}
