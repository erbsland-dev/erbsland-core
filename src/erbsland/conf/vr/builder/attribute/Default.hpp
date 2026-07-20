// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../DefaultValueFactory.hpp"

#include "../../../../text/StringList.hpp"

#include <vector>

namespace erbsland::conf::vr::builder {

/// Assigns a default value to a rule.
struct Default : Attribute {
    explicit Default(const impl::ValuePtr &value) : _value{value} {}
    explicit Default(const Integer value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const bool value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const Float value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const text::String &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const time::Date &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const time::Time &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const time::TimeWithZone &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const time::DateTime &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const mem::ByteBlock &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const time::CalendarDelta &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const re::RegExPtr &value) : _value{detail::createDefaultValue(value)} {}
    explicit Default(const std::vector<Integer> &values) : _value{detail::createDefaultValue(values)} {}
    explicit Default(const std::vector<bool> &values) : _value{detail::createDefaultValue(values)} {}
    explicit Default(const std::vector<Float> &values) : _value{detail::createDefaultValue(values)} {}
    explicit Default(const text::StringList &values) : _value{detail::createDefaultValue(values)} {}
    explicit Default(const std::vector<mem::ByteBlock> &values) : _value{detail::createDefaultValue(values)} {}
    explicit Default(const std::vector<std::vector<Integer>> &values) : _value{detail::createDefaultValue(values)} {}
    explicit Default(const std::vector<std::vector<Float>> &values) : _value{detail::createDefaultValue(values)} {}

    void operator()(impl::Rule &rule) override;

    impl::ValuePtr _value;
};

}
