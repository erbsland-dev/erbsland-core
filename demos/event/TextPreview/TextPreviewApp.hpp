// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all_core.hpp>
#include <erbsland/all_event.hpp>
#include <erbsland/all_options.hpp>
#include <erbsland/all_text.hpp>
#include <erbsland/Application.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/ExitCode.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <erbsland/Options.hpp>
#include <erbsland/String.hpp>

#include <cstdint>

namespace demo {

/// Keep preview state on main while a worker returns independently owned text values.
class TextPreviewApp final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    [[nodiscard]] auto main() -> el::ExitCode override;
    void cleanup() noexcept override;

private:
    void begin();
    void requestPreview(el::String text);
    void receivePreview(std::uint64_t requestId, el::String preview);

private:
    el::ManagedEventThreadPtr _worker; ///< Retained on main until application shutdown joins it.
    el::EventTimerPtr _deadline;       ///< Main-loop timer bounds the wait for replies.
    el::String _scenario;              ///< Configured before dispatch begins; read only on main.
    std::uint64_t _requestId{0};       ///< Latest request identity, owned by main.
    int _received{0};                  ///< All replies, including obsolete ones, owned by main.
    int _accepted{0};                  ///< Replies allowed to update the preview, owned by main.
    el::String _preview;               ///< Display state, owned by main.
};

}
