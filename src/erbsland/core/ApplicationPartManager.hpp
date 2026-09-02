// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ApplicationPartCallback.hpp"
#include "ApplicationPartManagerAccess.hpp"
#include "ApplicationPartTraits.hpp"

#include "../event/Events_fwd.hpp"
#include "../options/Options_fwd.hpp"
#include "../options/OptionValues_fwd.hpp"

#include <exception>
#include <functional>
#include <memory>
#include <utility>

namespace erbsland::core {

/// A detached manager for dependency-aware application parts.
/// @seedoc{/reference/core/application_framework}
/// @tested{ApplicationPartManagerTest ApplicationPartApplicationTest}
class ApplicationPartManager : public ApplicationPartManagerAccess {
    friend class Application;

protected:
    /// One deferred part registration.
    struct Registration {
        std::function<ApplicationPartIdentifierPtr()> identifier;    ///< Resolves the part identifier.
        std::function<ApplicationPartIdentifierList()> dependencies; ///< Resolves dependency identifiers.
        std::function<ApplicationPartPtr()> factory;                 ///< Creates the part instance.
    };

public:
    // defaults
    ~ApplicationPartManager() override = default;

public: // factory methods
    /// Create a detached application-part manager.
    /// If `controlEvents` is null, the manager creates and owns a dedicated control event thread.
    /// @param controlEvents Optional externally owned control event target.
    /// @return A new application-part manager.
    [[nodiscard]] static auto create(event::EventsPtr controlEvents = {}) -> ApplicationPartManagerPtr;

public: // registration and preparation
    /// Defer registration of an application-part class.
    /// @tparam T The concrete application-part class.
    /// @throws err::LogicError If preparation already started.
    template <ApplicationPartClass T>
    void registerPart() {
        registerPart(
            Registration{
                .identifier = []() -> ApplicationPartIdentifierPtr { return T::partIdentifier(); },
                .dependencies = []() -> ApplicationPartIdentifierList { return T::dependencies(); },
                .factory = []() -> ApplicationPartPtr { return T::create(); },
            });
    }
    /// Validate registrations, construct all parts, and enter `Ready`.
    virtual void prepare() = 0;
    /// Forward command-line option registration to prepared parts.
    /// @param options The shared option definitions.
    virtual void registerCommandLineOptions(const options::OptionsPtr &options) = 0;
    /// Forward successfully parsed command-line values to prepared parts.
    /// @param values The parsed option values.
    virtual void parseCommandLine(const options::OptionValuesPtr &values) = 0;

public: // lifecycle
    /// Begin initial automatic startup asynchronously.
    virtual void start() = 0;
    /// Start a part and its inactive dependency closure asynchronously.
    /// @param identifier The part to start.
    virtual void start(const ApplicationPartIdentifierPtr &identifier) = 0;
    /// Stop a part and its active dependent closure asynchronously.
    /// @param identifier The part to stop.
    virtual void stop(const ApplicationPartIdentifierPtr &identifier) = 0;
    /// Stop all parts asynchronously.
    virtual void stop() = 0;

public: // callbacks and errors
    /// Set the part-error policy callback.
    virtual void setErrorHandler(ApplicationPartErrorHandler handler) = 0;
    /// Set the manager-state callback.
    virtual void setStateChangedFn(ApplicationPartManagerStateChangedFn callback) = 0;
    /// Set the part-state callback.
    virtual void setPartStateChangedFn(ApplicationPartStateChangedFn callback) = 0;
    /// Test whether an error is queued.
    [[nodiscard]] virtual auto hasError() const noexcept -> bool = 0;
    /// Take the oldest queued error.
    [[nodiscard]] virtual auto takeError() noexcept -> std::exception_ptr = 0;

protected:
    /// Store one deferred part registration.
    /// @param registration The erased part metadata and factory.
    virtual void registerPart(Registration registration) = 0;

private:
    /// Set the owning framework's manager-state observer without replacing the user callback.
    virtual void setOwnerStateChangedFn(ApplicationPartManagerStateChangedFn callback) = 0;
};

}
