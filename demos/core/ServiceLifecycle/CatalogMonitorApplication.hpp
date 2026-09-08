// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/all.hpp>

namespace demo {

/// A long-running catalog monitor with explicit startup progress reporting.
///
/// Report startup as pending before operations that can exceed the Windows Service Control Manager timeout.
/// Refresh the estimate after each meaningful step and report completion exactly when the application can do useful
/// work. On Linux, macOS, and interactive Windows runs, the same calls are safe no-ops.
class CatalogMonitorApplication final : public el::Application {
public:
    using Application::Application;

protected: // implement Application
    void initialize() override;
    void cleanup() noexcept override;

private:
    void loadConfiguration();
    void openCatalog();
    void refreshCatalog();

private:
    el::EventTimerPtr _refreshTimer;
};

}
