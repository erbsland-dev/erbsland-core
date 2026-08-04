// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationInstanceBuilderBase.hpp"
#include "IllegalApplicationInstanceAccess.hpp"

#include <type_traits>

/// Concrete builder for configured application and data types.
template <
    typename tApplication = erbsland::core::Application,
    typename tApplicationData = erbsland::core::impl::ApplicationDataImpl>
    requires std::is_base_of_v<erbsland::core::Application, tApplication> &&
    std::is_base_of_v<erbsland::core::impl::ApplicationData, tApplicationData>
class ApplicationInstanceBuilder : public ApplicationInstanceBuilderBase {
public:
    // defaults
    ~ApplicationInstanceBuilder() override = default;

public:
    auto createApplication() -> std::unique_ptr<erbsland::core::Application> override {
        if constexpr (std::is_default_constructible_v<tApplication>) {
            return std::make_unique<tApplication>();
        } else {
            throw IllegalApplicationInstanceAccess{"The configured test application has no default constructor."};
        }
    }
    auto createApplication(int argc, char *argv[]) -> std::unique_ptr<erbsland::core::Application> override {
        if constexpr (std::is_constructible_v<tApplication, int, char **>) {
            return std::make_unique<tApplication>(argc, argv);
        } else {
            throw IllegalApplicationInstanceAccess{
                "The configured test application has no constructor for UTF-8 arguments."};
        }
    }
    auto createApplication(int argc, wchar_t *argv[]) -> std::unique_ptr<erbsland::core::Application> override {
        if constexpr (std::is_constructible_v<tApplication, int, wchar_t **>) {
            return std::make_unique<tApplication>(argc, argv);
        } else {
            throw IllegalApplicationInstanceAccess{
                "The configured test application has no constructor for wide arguments."};
        }
    }
    auto createApplicationData() -> erbsland::core::impl::ApplicationDataPtr override {
        return std::make_shared<tApplicationData>();
    }
};
