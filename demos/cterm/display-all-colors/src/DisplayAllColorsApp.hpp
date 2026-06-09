// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <TerminalApplication.hpp>

/// Display and label all available colors.
class DisplayAllColorsApp final : public TerminalApplication {
public:
    using TerminalApplication::TerminalApplication;

public:
    /// Render the color overview once and exit the demo.
    auto beforeMain() -> int override;

private:
    void renderTable();
    void renderMatrix();
    void renderRainbow();
};
