// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ApplicationPartManager.hpp"

#include "ApplicationPartIdentifier.hpp"

#include "../../err/LogicError.hpp"
#include "../../text/Literals.hpp"

#include <utility>

namespace erbsland::core::impl {

using namespace text::literals;

void ApplicationPartManager::registerPart(Registration registration) {
    const auto lock = std::scoped_lock{_mutex};
    if (_state != ApplicationPartManagerState::Uninitialized || _prepared) {
        throw err::LogicError{"Can not register an application part after preparation started."_el};
    }
    _registrations.emplace_back(std::move(registration));
}

void ApplicationPartManager::prepare() {
    {
        const auto lock = std::scoped_lock{_mutex};
        if (_state != ApplicationPartManagerState::Uninitialized || _prepared) {
            throw err::LogicError{"The application-part manager can only be prepared once."_el};
        }
    }
    try {
        auto records = std::vector<ApplicationPartRecord>{};
        auto nameToNumber = text::StringHashMap<std::size_t>{};
        records.reserve(_registrations.size());
        for (const auto &registration : _registrations) {
            const auto identifier = registration.identifier();
            if (identifier == nullptr) {
                throw err::LogicError{"An application part returned a null identifier."_el};
            }
            if (nameToNumber.contains(identifier->name())) {
                throw err::LogicError{"An application-part identifier name is registered more than once."_el};
            }
            const auto number = records.size() + 1;
            nameToNumber.set(identifier->name(), number);
            if (const auto implementation = std::dynamic_pointer_cast<impl::ApplicationPartIdentifier>(identifier);
                implementation != nullptr) {
                implementation->setCachedNumber(_managerToken, number);
            }
            auto record = ApplicationPartRecord{};
            record.identifier = identifier;
            records.emplace_back(std::move(record));
        }
        for (std::size_t index = 0; index < _registrations.size(); ++index) {
            const auto dependencies = _registrations[index].dependencies();
            for (const auto &identifier : dependencies) {
                if (identifier == nullptr) {
                    throw err::LogicError{"An application part returned a null dependency identifier."_el};
                }
                const auto dependencyNumber = nameToNumber.get(identifier->name());
                if (!dependencyNumber.has_value()) {
                    throw err::LogicError{"An application-part dependency is not registered."_el};
                }
                if (*dependencyNumber == index + 1) {
                    throw err::LogicError{"An application part can not depend on itself."_el};
                }
                records[index].dependencies.emplace_back(*dependencyNumber);
                records[*dependencyNumber - 1].dependents.emplace_back(index + 1);
                if (const auto implementation = std::dynamic_pointer_cast<impl::ApplicationPartIdentifier>(identifier);
                    implementation != nullptr) {
                    implementation->setCachedNumber(_managerToken, *dependencyNumber);
                }
            }
        }
        {
            const auto lock = std::scoped_lock{_mutex};
            _records = std::move(records);
            _nameToNumber = std::move(nameToNumber);
        }
        validateAcyclicGraph();
        const auto managerAccess = ApplicationPartManagerAccessWeakPtr{shared_from_this()};
        for (std::size_t index = 0; index < _registrations.size(); ++index) {
            auto instance = _registrations[index].factory();
            if (instance == nullptr) {
                throw err::LogicError{"An application-part factory returned a null instance."_el};
            }
            instance->bind(managerAccess, {});
            const auto lock = std::scoped_lock{_mutex};
            _records[index].part = std::move(instance);
        }
        {
            const auto lock = std::scoped_lock{_mutex};
            _prepared = true;
            _registrations.clear();
        }
        setManagerState(ApplicationPartManagerState::Ready);
    } catch (...) {
        addError(std::current_exception());
        setManagerState(ApplicationPartManagerState::Failed);
        throw;
    }
}

void ApplicationPartManager::registerCommandLineOptions(const options::OptionsPtr &options) {
    auto parts = std::vector<ApplicationPartPtr>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (!_prepared) {
            throw err::LogicError{"The application-part manager must be prepared before option registration."_el};
        }
        for (const auto &record : _records) {
            parts.emplace_back(record.part);
        }
    }
    for (const auto &part : parts) {
        static_cast<ApplicationPartCommandLine &>(*part).registerCommandLineOptions(options);
    }
}

void ApplicationPartManager::parseCommandLine(const options::OptionValuesPtr &values) {
    auto parts = std::vector<ApplicationPartPtr>{};
    {
        const auto lock = std::scoped_lock{_mutex};
        if (!_prepared) {
            throw err::LogicError{"The application-part manager must be prepared before option parsing."_el};
        }
        for (const auto &record : _records) {
            parts.emplace_back(record.part);
        }
    }
    for (const auto &part : parts) {
        static_cast<ApplicationPartCommandLine &>(*part).parseCommandLine(values);
    }
}

auto ApplicationPartManager::resolveIdentifier(const ApplicationPartIdentifierPtr &identifier) const -> std::size_t {
    const auto lock = std::scoped_lock{_mutex};
    return resolveIdentifierLocked(identifier);
}

auto ApplicationPartManager::resolveIdentifierLocked(const ApplicationPartIdentifierPtr &identifier) const
    -> std::size_t {
    if (identifier == nullptr) {
        throw err::LogicError{"The application-part identifier must not be null."_el};
    }
    if (const auto implementation = std::dynamic_pointer_cast<impl::ApplicationPartIdentifier>(identifier);
        implementation != nullptr) {
        const auto cachedNumber = implementation->cachedNumber(_managerToken);
        if (cachedNumber > 0 && cachedNumber <= _records.size() &&
            _records[cachedNumber - 1].identifier->name() == identifier->name()) {
            return cachedNumber;
        }
    }
    const auto number = _nameToNumber.get(identifier->name());
    if (!number.has_value()) {
        throw err::LogicError{"The application part is not registered in this manager."_el};
    }
    if (const auto implementation = std::dynamic_pointer_cast<impl::ApplicationPartIdentifier>(identifier);
        implementation != nullptr) {
        implementation->setCachedNumber(_managerToken, *number);
    }
    return *number;
}

void ApplicationPartManager::validateAcyclicGraph() const {
    auto marks = std::vector<uint8_t>(_records.size(), 0);
    for (std::size_t number = 1; number <= _records.size(); ++number) {
        validateAcyclicPart(number, marks);
    }
}

void ApplicationPartManager::validateAcyclicPart(const std::size_t number, std::vector<uint8_t> &marks) const {
    auto &mark = marks[number - 1];
    if (mark == 2) {
        return;
    }
    if (mark == 1) {
        throw err::LogicError{"The application-part dependency graph contains a cycle."_el};
    }
    mark = 1;
    for (const auto dependency : _records[number - 1].dependencies) {
        validateAcyclicPart(dependency, marks);
    }
    mark = 2;
}

}
