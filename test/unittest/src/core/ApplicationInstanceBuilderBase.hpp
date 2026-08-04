// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/Application.hpp>
#include <erbsland/core/impl/ApplicationDataImpl.hpp>

#include <memory>

/// Builder base to create the application and data instance.
class ApplicationInstanceBuilderBase {
public:
    // defaults
    virtual ~ApplicationInstanceBuilderBase() = default;

public:
    /// Create the default application instance.
    virtual auto createApplication() -> std::unique_ptr<erbsland::core::Application> = 0;
    /// Create an application instance from UTF-8 arguments.
    virtual auto createApplication(int argc, char *argv[]) -> std::unique_ptr<erbsland::core::Application> = 0;
    /// Create an application instance from wide-character arguments.
    virtual auto createApplication(int argc, wchar_t *argv[]) -> std::unique_ptr<erbsland::core::Application> = 0;
    /// Create the application data instance.
    virtual auto createApplicationData() -> erbsland::core::impl::ApplicationDataPtr = 0;
};
