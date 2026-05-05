// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BufferTestHelper.hpp"
#include "TerminalTestBackend.hpp"

#include <memory>

class TerminalTestHelper : public BufferTestHelper {
public:
    auto createTerminal(
        const std::shared_ptr<TerminalTestBackend> &backend, const bgeo::BlockSize size = bgeo::BlockSize{80, 25})
        -> std::unique_ptr<Terminal> {
        auto terminal = std::make_unique<Terminal>(size);
        terminal->setBackend(backend);
        return terminal;
    }

    auto createMinimumSizeWarningSettings(const bgeo::BlockSize minimumSize, const erbsland::text::StringView &message)
        -> UpdateSettings {
        auto settings = UpdateSettings{};
        settings.setMinimumSize(minimumSize);
        settings.setMinimumSizeBackground(Block{U'.'});
        settings.setMinimumSizeMessage(BlockString{message});
        settings.setSwitchToAlternateBuffer(false);
        return settings;
    }
};
