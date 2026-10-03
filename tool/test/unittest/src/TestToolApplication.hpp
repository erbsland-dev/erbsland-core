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
    using ToolApplication::ToolApplication;

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
