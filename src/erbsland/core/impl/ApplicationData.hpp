// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData_fwd.hpp"

#include "application_data/ApplicationCryptologyData_fwd.hpp"
#include "application_data/ApplicationDataAccessor.hpp"
#include "application_data/ApplicationEventData_fwd.hpp"
#include "application_data/ApplicationLifecycleData_fwd.hpp"
#include "application_data/ApplicationLogData_fwd.hpp"
#include "application_data/ApplicationOptionsData_fwd.hpp"
#include "application_data/ApplicationPartsData_fwd.hpp"
#include "application_data/ApplicationRandomData_fwd.hpp"
#include "application_data/ApplicationResourceData_fwd.hpp"
#include "application_data/ApplicationRuntimeData_fwd.hpp"
#include "application_data/ApplicationSystemData_fwd.hpp"
#include "application_data/ApplicationTerminalData_fwd.hpp"

#include <memory>

namespace erbsland::core::impl {

/// Lazy accessor for runtime application data.
using ApplicationRuntimeDataAccessor = ApplicationDataAccessor<ApplicationRuntimeData>;
/// Lazy accessor for command-line option data.
using ApplicationOptionsDataAccessor = ApplicationDataAccessor<ApplicationOptionsData>;
/// Lazy accessor for application lifecycle data.
using ApplicationLifecycleDataAccessor = ApplicationDataAccessor<ApplicationLifecycleData>;
/// Lazy accessor for application-part data.
using ApplicationPartsDataAccessor = ApplicationDataAccessor<ApplicationPartsData>;
/// Lazy accessor for application event data.
using ApplicationEventDataAccessor = ApplicationDataAccessor<ApplicationEventData>;
/// Lazy accessor for terminal and system-output data.
using ApplicationTerminalDataAccessor = ApplicationDataAccessor<ApplicationTerminalData>;
/// Lazy accessor for application logging data.
using ApplicationLogDataAccessor = ApplicationDataAccessor<ApplicationLogData>;
/// Lazy accessor for application random-generator data.
using ApplicationRandomDataAccessor = ApplicationDataAccessor<ApplicationRandomData>;
/// Lazy accessor for application cryptology data.
using ApplicationCryptologyDataAccessor = ApplicationDataAccessor<ApplicationCryptologyData>;
/// Lazy accessor for compiled-resource data.
using ApplicationResourceDataAccessor = ApplicationDataAccessor<ApplicationResourceData>;
/// Lazy accessor for system-service data.
using ApplicationSystemDataAccessor = ApplicationDataAccessor<ApplicationSystemData>;

/// Composition interface for the internal application data.
/// This shared object is the actual application singleton and can outlive individual `Application` facades.
/// @tested{ApplicationDataAccessorTest ApplicationTestScopeTest}
class ApplicationData : public std::enable_shared_from_this<ApplicationData> {
public:
    // defaults
    ApplicationData() = default;
    virtual ~ApplicationData() = default;

public: // component accessors
    /// Access lazy runtime data.
    [[nodiscard]] virtual auto runtime() const noexcept -> const ApplicationRuntimeDataAccessor & = 0;
    /// Access lazy command-line option data.
    [[nodiscard]] virtual auto options() const noexcept -> const ApplicationOptionsDataAccessor & = 0;
    /// Access lazy lifecycle data.
    [[nodiscard]] virtual auto lifecycle() const noexcept -> const ApplicationLifecycleDataAccessor & = 0;
    /// Access lazy application-part data.
    [[nodiscard]] virtual auto parts() const noexcept -> const ApplicationPartsDataAccessor & = 0;
    /// Access lazy event data.
    [[nodiscard]] virtual auto events() const noexcept -> const ApplicationEventDataAccessor & = 0;
    /// Access lazy terminal data.
    [[nodiscard]] virtual auto terminal() const noexcept -> const ApplicationTerminalDataAccessor & = 0;
    /// Access lazy logging data.
    [[nodiscard]] virtual auto logging() const noexcept -> const ApplicationLogDataAccessor & = 0;
    /// Access lazy random-generator data.
    [[nodiscard]] virtual auto random() const noexcept -> const ApplicationRandomDataAccessor & = 0;
    /// Access lazy cryptology data.
    [[nodiscard]] virtual auto cryptology() const noexcept -> const ApplicationCryptologyDataAccessor & = 0;
    /// Access lazy compiled-resource data.
    [[nodiscard]] virtual auto resources() const noexcept -> const ApplicationResourceDataAccessor & = 0;
    /// Access lazy system-service data.
    [[nodiscard]] virtual auto system() const noexcept -> const ApplicationSystemDataAccessor & = 0;
};

}
