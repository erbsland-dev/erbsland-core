// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Definitions.hpp"
#include "ProfilingApplication_fwd.hpp"
#include "ProfilingDefinition.hpp"
#include "ProfilingDefinition_fwd.hpp"

#include <erbsland/all.hpp>

namespace erbsland::profiling {

/// Common command-line application base for profiling tools.
/// @tested{ProfilingApplicationTest}
class ProfilingApplication : public Application {
public:
    using Application::Application;

protected:
    /// Populate the declarative profiling definition.
    virtual void configureProfiling(ProfilingDefinition &definition) = 0;
    /// Register tool-specific command-line filters and controls.
    virtual void registerProfilingOptions(const OptionsPtr &options);
    /// Execute the domain profiler after common option registration.
    [[nodiscard]] virtual auto executeProfiling() -> ExitCode = 0;

protected: // implement Application
    /// Initialize the profiling definition and runtime.
    void initialize() final;
    /// Register common and tool-specific profiling options.
    void registerCommandLineOptions(const OptionsPtr &options) final;
    /// Run the configured profiler.
    [[nodiscard]] auto main() -> ExitCode final;

protected: // access
    /// Access the configured profiling definition.
    [[nodiscard]] auto profilingDefinition() const -> const ProfilingDefinition & { return _definition; }
    /// Execute the declaratively registered scenarios with common options.
    [[nodiscard]] auto runRegisteredProfiling() -> ExitCode;

private:
    /// Parse an ELCL time-delta command-line value.
    [[nodiscard]] static auto parseTimeDeltaOverride(const String &value) -> TimeDelta;
    /// Parse an ELCL byte-count command-line value.
    [[nodiscard]] static auto parseByteLengthOverride(const String &value) -> ByteLength;

private:
    ProfilingDefinition _definition;
};

}
