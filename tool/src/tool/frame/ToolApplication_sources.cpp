// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ToolApplication.hpp"

#include <erbsland/RegEx.hpp>
#include <erbsland/StringFormat.hpp>

#include <utility>

namespace tool::frame {

void ToolApplication::requireOpenRegistration() const {
    if (_registrationClosed) {
        throwRegistrationFailure("Registration is closed after command-line processing starts."_el);
    }
}

void ToolApplication::validateIdentifier(const el::String &identifier) {
    static const auto pattern = el::re::RegEx::compile("[a-z]+(?:_[a-z]+)*"_el);
    if (!pattern->fullMatch(identifier)) {
        throwRegistrationFailure(
            el::StringFormat{"Identifier \"{}\" is not valid."_el}.build(identifier.toSafeString(el::CpCount{100})));
    }
}

void ToolApplication::requireAvailableIdentifier(const el::String &identifier) const {
    if (identifier == "rendering"_el) {
        throwRegistrationFailure("The rendering configuration identifier is reserved."_el);
    }
    for (const auto &entry : _sources) {
        if (entry.identifier == identifier) {
            throwRegistrationFailure("An object with this identifier is already registered."_el);
        }
    }
    for (const auto &task : _tasks) {
        if (task->identifier() == identifier) {
            throwRegistrationFailure("An object with this identifier is already registered."_el);
        }
    }
    for (const auto &tool : _tools) {
        if (tool->identifier() == identifier) {
            throwRegistrationFailure("An object with this identifier is already registered."_el);
        }
    }
}

void ToolApplication::registerDataSource(el::String identifier, std::unique_ptr<DataSource> source) {
    validateIdentifier(identifier);
    requireAvailableIdentifier(identifier);
    _sources.push_back(impl::DataSourceRegistration{std::move(identifier), std::move(source)});
}

auto ToolApplication::sourceRegistration(const el::String &identifier) -> impl::DataSourceRegistration & {
    for (auto &entry : _sources) {
        if (entry.identifier == identifier) {
            return entry;
        }
    }
    throwSourceAccessFailure(identifier, "The source is not registered."_el);
}

auto ToolApplication::initializedSource(const el::String &identifier) -> DataSource & {
    auto &entry = sourceRegistration(identifier);
    if (!entry.initialized) {
        throwSourceAccessFailure(identifier, "The source has not completed initialization."_el);
    }
    return *entry.source;
}

void ToolApplication::throwSourceAccessFailure(const el::String &identifier, const el::String &description) {
    throw el::LogicFailure{el::StringFormat{"Data source '{}': {}"_el}.build(identifier, description)};
}

auto ToolApplication::validateSection(
    const el::conf::DocumentPtr &configuration, const el::String &identifier, const el::conf::vr::RulesPtr &rules)
    -> el::conf::ValuePtr {
    const auto section = configuration->value(identifier);
    if (rules) {
        if (!section) {
            throwConfigurationError(
                configuration,
                el::StringFormat{"'{}' requires a configuration section with its name."_el}.build(identifier));
        }
        if (!section->isSectionWithNames()) {
            throwConfigurationError(
                section,
                el::StringFormat{"The configuration value '{}' must be a regular section."_el}.build(identifier));
        }
        rules->validate(section, 1);
    } else if (section) {
        throwConfigurationError(
            section, el::StringFormat{"'{}' must not have a configuration section."_el}.build(identifier));
    }
    return section;
}

auto ToolApplication::validateSources(const el::conf::DocumentPtr &configuration, const el::StringList &identifiers)
    -> el::StringList {
    auto selected = el::StringList{};
    // Resolve all dependencies and validate all sections before the first source callback.
    for (const auto &identifier : identifiers) {
        if (selected.contains(identifier)) {
            continue;
        }
        auto &entry = sourceRegistration(identifier);
        entry.configuration = validateSection(configuration, identifier, entry.source->configurationRules());
        selected.append(identifier);
    }
    return selected;
}

void ToolApplication::initializeSources(const el::StringList &identifiers, const el::LogStreamPtr &log) {
    for (const auto &identifier : identifiers) {
        auto &entry = sourceRegistration(identifier);
        entry.source->setLog(log);
        if (!entry.initialized) {
            entry.source->configure(entry.configuration);
        }
    }
    for (const auto &identifier : identifiers) {
        auto &entry = sourceRegistration(identifier);
        if (!entry.initialized) {
            entry.source->initialize();
            entry.initialized = true;
        }
    }
}

}
