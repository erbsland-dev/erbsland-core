// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InboxWatcher.hpp"

#include <erbsland/Application.hpp>
#include <erbsland/ItemCount.hpp>

namespace demo {

/// Report newly visible files in an inbox directory.
class InboxMonitorApp final : public el::Application {
    static constexpr auto cDirectory = el::StringLiteral{"directory"};
    static constexpr auto cOnce = el::StringLiteral{"once"};

public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void registerCommandLineOptions(const el::OptionsPtr &options) override;
    void handleCommandLineOptions(const el::OptionValuesPtr &values) override;

private:
    void watch(const el::Path &directory, bool once);
    el::ItemCount _arrivals;                ///< Number of files announced.
    std::shared_ptr<InboxWatcher> _watcher; ///< Keeps the source alive between scans.
};

}
