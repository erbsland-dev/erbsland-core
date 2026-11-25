// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CapturedTerminalBackend.hpp"

#include <erbsland/cterm/Terminal.hpp>
#include <erbsland/StandardStreams.hpp>
#include <tool/frame/ToolApplication.hpp>

#include <memory>

/// Run the real tool lifecycle with captured, noninteractive terminal output.
/// @notest{Unit-test fixture; exercised by ToolApplicationTest.}
class TestToolApplication final : public tool::frame::ToolApplication {
public:
    /// Create the Core maintenance application with a captured terminal.
    /// @param argc Native argument count.
    /// @param argv Native arguments.
    TestToolApplication(int argc, char *argv[]) : ToolApplication{argc, argv, coreProject()} {}

public: // fixtures
    /// Get the identity of the project used by Core's maintenance fixtures.
    [[nodiscard]] static auto coreProject() -> tool::frame::ToolProject {
        using namespace el::text::literals;
        return {
            "Erbsland Core"_el, "erbsland-core"_el, "erbsland-core-tool"_el, "tool/conf/erbsland-core-tool.elcl"_el};
    }

protected: // implement Application
    auto createAndInitializeTerminal(
        [[maybe_unused]] el::block::Size size, [[maybe_unused]] el::cterm::TerminalFlags flags)
        -> el::cterm::TerminalPtr override {
        auto terminal =
            std::make_shared<el::cterm::Terminal>(std::make_shared<CapturedTerminalBackend>(el::stream::stdOut()));
        terminal->initializeScreen();
        return terminal;
    }
};
