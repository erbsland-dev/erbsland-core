// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AxisDefinition.hpp"
#include "Definitions.hpp"
#include "FunctionalityDefinition.hpp"
#include "MetricDefinition.hpp"
#include "ParameterDefinition.hpp"
#include "ProfilingDefinition_fwd.hpp"
#include "SuiteDefinition.hpp"

#include <erbsland/all.hpp>

#include <functional>

namespace erbsland::profiling {

/// Declarative definition of one profiling application.
/// @tested{ProfilingDefinitionTest}
class ProfilingDefinition final {
public:
    /// Set application metadata.
    auto setApplication(String name, Version version, String helpTitle, String helpDescription)
        -> ProfilingDefinition &;
    /// Set the embedded default ELCL configuration.
    auto setDefaultConfiguration(String value) -> ProfilingDefinition &;
    /// Add a categorical axis.
    auto addAxis(AxisDefinition value) -> ProfilingDefinition &;
    /// Add a typed scenario parameter.
    auto addParameter(ParameterDefinition value) -> ProfilingDefinition &;
    /// Add a measurable functionality.
    auto addFunctionality(FunctionalityDefinition value) -> ProfilingDefinition &;
    /// Add a built-in scenario suite.
    auto addSuite(SuiteDefinition value) -> ProfilingDefinition &;
    /// Add a reported metric.
    auto addMetric(MetricDefinition value) -> ProfilingDefinition &;
    /// Set the expanded-scenario compatibility predicate.
    auto setCompatibility(std::function<bool(const Scenario &)> value) -> ProfilingDefinition &;

public: // lookup
    [[nodiscard]] auto findAxis(const String &id) const -> const AxisDefinition *;
    [[nodiscard]] auto findParameter(const String &id) const -> const ParameterDefinition *;
    [[nodiscard]] auto findFunctionality(const String &id) const -> const FunctionalityDefinition *;
    [[nodiscard]] auto findSuite(const String &id) const -> const SuiteDefinition *;
    /// Test whether an expanded scenario is compatible with domain rules.
    [[nodiscard]] auto isCompatible(const Scenario &scenario) const -> bool;

public: // attributes
    [[nodiscard]] auto applicationName() const -> const String & { return _applicationName; }
    [[nodiscard]] auto applicationVersion() const noexcept -> Version { return _applicationVersion; }
    [[nodiscard]] auto helpTitle() const -> const String & { return _helpTitle; }
    [[nodiscard]] auto helpDescription() const -> const String & { return _helpDescription; }
    [[nodiscard]] auto defaultConfiguration() const -> const String & { return _defaultConfiguration; }
    [[nodiscard]] auto axes() const -> const List<AxisDefinition> & { return _axes; }
    [[nodiscard]] auto parameters() const -> const List<ParameterDefinition> & { return _parameters; }
    [[nodiscard]] auto functionalities() const -> const List<FunctionalityDefinition> & { return _functionalities; }
    [[nodiscard]] auto suites() const -> const List<SuiteDefinition> & { return _suites; }
    [[nodiscard]] auto metrics() const -> const List<MetricDefinition> & { return _metrics; }

private:
    String _applicationName;
    Version _applicationVersion;
    String _helpTitle;
    String _helpDescription;
    String _defaultConfiguration;
    List<AxisDefinition> _axes;
    List<ParameterDefinition> _parameters;
    List<FunctionalityDefinition> _functionalities;
    List<SuiteDefinition> _suites;
    List<MetricDefinition> _metrics;
    std::function<bool(const Scenario &)> _compatibility;
};

}
