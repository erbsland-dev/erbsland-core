// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "QueryEditor.hpp"
#include "SearchResults.hpp"
#include "SearchView.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ElapsedTimer.hpp>
#include <erbsland/EventTimer.hpp>
#include <erbsland/ManagedEventThread.hpp>
#include <erbsland/Options.hpp>

#include <atomic>
#include <memory>

namespace demo {

/// Search a directory while editing a glob and selecting a matching path on main.
class FileSearchApp final : public el::Application {
    static constexpr auto cDirectoryOption = el::StringLiteral{"directory"};
    using Generation = std::atomic<std::uint64_t>;
    using GenerationPtr = std::shared_ptr<Generation>;

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;
    void cleanup() noexcept override;

private:
    void begin();
    void tick();
    void queryChanged();
    void submitSearch();
    void requestExit(bool select);

private:
    el::Path _root;                                              ///< Absolute root, fixed before dispatch.
    QueryEditor _query;                                          ///< Main-loop input state.
    SearchResults _results;                                      ///< Main-loop result state.
    SearchView _view;                                            ///< Main-loop presentation.
    el::ManagedEventThreadPtr _worker;                           ///< Retained until Application joins it.
    el::EventTimerPtr _refresh;                                  ///< Polls keys and renders at approximately 30 Hz.
    el::EventTimerPtr _debounce;                                 ///< Starts only the latest edited query.
    el::ElapsedTimer _blink;                                     ///< Cursor phase, reset by keyboard input.
    GenerationPtr _generation = std::make_shared<Generation>(0); ///< The worker reads cancellation, not UI state.
    el::block::Size _lastSize;                                   ///< Screen size of the last rendered frame.
    bool _dirty{true};                                           ///< Input or results changed since rendering.
    bool _cursorVisible{false};                                  ///< Cursor phase of the last rendered frame.
    bool _exiting{false};                                        ///< Suppresses late rendering and result deliveries.
};

}
