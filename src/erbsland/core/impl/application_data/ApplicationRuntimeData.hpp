// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationRuntimeData_fwd.hpp"

#include "../../ApplicationInfo.hpp"
#include "../../InitializeFn.hpp"
#include "../../MainFn.hpp"

#include <atomic>
#include <utility>

namespace erbsland::core::impl {

/// Data and callbacks for the application runtime.
/// @tested{ApplicationOptionsTest ApplicationServiceLifecycleTest ApplicationTestScopeTest}
class ApplicationRuntimeData final {
public:
    /// Record that application execution started.
    void startRun() noexcept { _runStarted.store(true, std::memory_order_release); }
    /// Test whether application execution started.
    [[nodiscard]] auto isRunStarted() const noexcept -> bool { return _runStarted.load(std::memory_order_acquire); }
    /// Access mutable application information.
    [[nodiscard]] auto info() noexcept -> ApplicationInfo & { return _info; }
    /// Access application information.
    [[nodiscard]] auto info() const noexcept -> const ApplicationInfo & { return _info; }
    /// Access the initialization callback.
    [[nodiscard]] auto initializeFn() noexcept -> const InitializeFn & { return _initializeFn; }
    /// Replace the initialization callback.
    void setInitializeFn(InitializeFn initializeFn) noexcept { _initializeFn = std::move(initializeFn); }
    /// Access the main callback.
    [[nodiscard]] auto mainFn() noexcept -> const MainFn & { return _mainFn; }
    /// Replace the main callback.
    void setMainFn(MainFn mainFn) noexcept { _mainFn = std::move(mainFn); }

private:
    std::atomic<bool> _runStarted{false}; ///< Whether application execution started.
    ApplicationInfo _info;                ///< Application metadata.
    InitializeFn _initializeFn;           ///< Lambda-based override of `Application::initialize()`.
    MainFn _mainFn;                       ///< Lambda-based override of `Application::main()`.
};

}
