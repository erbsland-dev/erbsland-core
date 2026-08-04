// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferTestHelper.hpp"
#include "TerminalTestBackend.hpp"

#include <memory>

/// Helper for constructing configured terminal test fixtures.
/// @notest{Used only by terminal unit tests.}
class TerminalTestHelper : public BufferTestHelper {
public:
    /// Create a terminal backed by the specified test backend.
    auto createTerminal(
        const std::shared_ptr<TerminalTestBackend> &backend, const bgeo::BlockSize size = bgeo::BlockSize{80, 25})
        -> std::unique_ptr<Terminal> {
        auto terminal = std::make_unique<Terminal>(size);
        terminal->setBackend(backend);
        return terminal;
    }

    /// Create update settings that show a minimum-size warning.
    auto createMinimumSizeWarningSettings(const bgeo::BlockSize minimumSize, const erbsland::text::String &message)
        -> UpdateSettings {
        auto settings = UpdateSettings{};
        settings.setMinimumSize(minimumSize);
        settings.setMinimumSizeBackground(Block{U'.'});
        settings.setMinimumSizeMessage(BlockStringEditor{message});
        settings.setSwitchToAlternateBuffer(false);
        return settings;
    }
};
