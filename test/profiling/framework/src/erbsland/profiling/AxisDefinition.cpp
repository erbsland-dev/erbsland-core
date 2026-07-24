// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AxisDefinition.hpp"

namespace erbsland::profiling {

AxisDefinition::AxisDefinition(String id, String configurationName, String optionName) :
    _id{std::move(id)}, _configurationName{std::move(configurationName)}, _optionName{std::move(optionName)} {
}

auto AxisDefinition::addValue(AxisValue value) -> AxisDefinition & {
    if (hasValue(value.id)) {
        throw ApplicationError{StringFormat{"Duplicate profiling axis value '{}'."}.build(value.id)};
    }
    _values.append(std::move(value));
    return *this;
}

auto AxisDefinition::hasValue(const String &id) const -> bool {
    for (const auto &value : _values) {
        if (value.id == id) {
            return true;
        }
    }
    return false;
}

}
