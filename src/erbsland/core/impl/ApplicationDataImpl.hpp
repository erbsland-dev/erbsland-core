// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationData.hpp"

namespace erbsland::core::impl {

/// Default composition of lazily created internal application-data components.
/// @tested{ApplicationDataAccessorTest ApplicationTestScopeTest}
class ApplicationDataImpl : public ApplicationData {
public:
    /// Create an empty application-data composition.
    ApplicationDataImpl();

    // defaults
    ~ApplicationDataImpl() override;

public: // implement ApplicationData
    [[nodiscard]] auto runtime() const noexcept -> const ApplicationRuntimeDataAccessor & override;
    [[nodiscard]] auto options() const noexcept -> const ApplicationOptionsDataAccessor & override;
    [[nodiscard]] auto lifecycle() const noexcept -> const ApplicationLifecycleDataAccessor & override;
    [[nodiscard]] auto parts() const noexcept -> const ApplicationPartsDataAccessor & override;
    [[nodiscard]] auto events() const noexcept -> const ApplicationEventDataAccessor & override;
    [[nodiscard]] auto terminal() const noexcept -> const ApplicationTerminalDataAccessor & override;
    [[nodiscard]] auto logging() const noexcept -> const ApplicationLogDataAccessor & override;
    [[nodiscard]] auto random() const noexcept -> const ApplicationRandomDataAccessor & override;
    [[nodiscard]] auto cryptology() const noexcept -> const ApplicationCryptologyDataAccessor & override;
    [[nodiscard]] auto resources() const noexcept -> const ApplicationResourceDataAccessor & override;
    [[nodiscard]] auto system() const noexcept -> const ApplicationSystemDataAccessor & override;

private:
    ApplicationRuntimeDataAccessor _runtime;       ///< Runtime data accessor.
    ApplicationOptionsDataAccessor _options;       ///< Option data accessor.
    ApplicationLifecycleDataAccessor _lifecycle;   ///< Lifecycle data accessor.
    ApplicationPartsDataAccessor _parts;           ///< Application-part data accessor.
    ApplicationEventDataAccessor _events;          ///< Event data accessor.
    ApplicationTerminalDataAccessor _terminal;     ///< Terminal data accessor.
    ApplicationLogDataAccessor _logging;           ///< Logging data accessor.
    ApplicationRandomDataAccessor _random;         ///< Random-generator data accessor.
    ApplicationCryptologyDataAccessor _cryptology; ///< Cryptology data accessor.
    ApplicationResourceDataAccessor _resources;    ///< Resource data accessor.
    ApplicationSystemDataAccessor _system;         ///< System-service data accessor.
};

}
