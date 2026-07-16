// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ElGrepApp.hpp"

#include <erbsland/cterm/Terminal.hpp>


auto main(const int argc, char *argv[]) -> int {
    auto app = elgrep::ElGrepApp{argc, argv};
    app.enableTerminal();
    if (!app.terminal()->isInteractive()) {
        app.terminal()->setOutputMode(el::cterm::Terminal::OutputMode::BlockText);
    }
    return app.run();
}
