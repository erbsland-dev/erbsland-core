// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ProfilingDefinition.hpp"

namespace erbsland::profiling {

using namespace text::literals;

auto ProfilingDefinition::setApplication(String name, Version version, String helpTitle, String helpDescription)
    -> ProfilingDefinition & {
    _applicationName = std::move(name);
    _applicationVersion = version;
    _helpTitle = std::move(helpTitle);
    _helpDescription = std::move(helpDescription);
    return *this;
}

auto ProfilingDefinition::setDefaultConfiguration(String value) -> ProfilingDefinition & {
    _defaultConfiguration = std::move(value);
    return *this;
}

auto ProfilingDefinition::addAxis(AxisDefinition value) -> ProfilingDefinition & {
    if (findAxis(value.id()) != nullptr) {
        throw ApplicationError{StringFormat{"Duplicate profiling axis '{}'."_el}.build(value.id())};
    }
    _axes.append(std::move(value));
    return *this;
}

auto ProfilingDefinition::addParameter(ParameterDefinition value) -> ProfilingDefinition & {
    if (findParameter(value.id) != nullptr) {
        throw ApplicationError{StringFormat{"Duplicate profiling parameter '{}'."_el}.build(value.id)};
    }
    _parameters.append(std::move(value));
    return *this;
}

auto ProfilingDefinition::addFunctionality(FunctionalityDefinition value) -> ProfilingDefinition & {
    if (findFunctionality(value.id) != nullptr) {
        throw ApplicationError{StringFormat{"Duplicate profiling functionality '{}'."_el}.build(value.id)};
    }
    _functionalities.append(std::move(value));
    return *this;
}

auto ProfilingDefinition::addSuite(SuiteDefinition value) -> ProfilingDefinition & {
    if (findSuite(value.id) != nullptr) {
        throw ApplicationError{StringFormat{"Duplicate profiling suite '{}'."_el}.build(value.id)};
    }
    _suites.append(std::move(value));
    return *this;
}

auto ProfilingDefinition::addMetric(MetricDefinition value) -> ProfilingDefinition & {
    for (const auto &metric : _metrics) {
        if (metric.id == value.id) {
            throw ApplicationError{StringFormat{"Duplicate profiling metric '{}'."_el}.build(value.id)};
        }
    }
    _metrics.append(std::move(value));
    return *this;
}

auto ProfilingDefinition::setCompatibility(std::function<bool(const Scenario &)> value) -> ProfilingDefinition & {
    _compatibility = std::move(value);
    return *this;
}

auto ProfilingDefinition::findAxis(const String &id) const -> const AxisDefinition * {
    for (const auto &value : _axes) {
        if (value.id() == id) {
            return &value;
        }
    }
    return nullptr;
}

auto ProfilingDefinition::findParameter(const String &id) const -> const ParameterDefinition * {
    for (const auto &value : _parameters) {
        if (value.id == id) {
            return &value;
        }
    }
    return nullptr;
}

auto ProfilingDefinition::findFunctionality(const String &id) const -> const FunctionalityDefinition * {
    for (const auto &value : _functionalities) {
        if (value.id == id) {
            return &value;
        }
    }
    return nullptr;
}

auto ProfilingDefinition::findSuite(const String &id) const -> const SuiteDefinition * {
    for (const auto &value : _suites) {
        if (value.id == id) {
            return &value;
        }
    }
    return nullptr;
}

auto ProfilingDefinition::isCompatible(const Scenario &scenario) const -> bool {
    return !_compatibility || _compatibility(scenario);
}

}
