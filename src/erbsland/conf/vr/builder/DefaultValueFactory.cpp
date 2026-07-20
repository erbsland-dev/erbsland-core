// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DefaultValueFactory.hpp"

#include "../../../err/ParameterError.hpp"

#include <ranges>

namespace erbsland::conf::vr::builder::detail {

namespace {

template <typename Range>
auto createScalarListValue(const Range &values) -> impl::ValuePtr {
    std::vector<impl::ValuePtr> list;
    list.reserve(static_cast<std::size_t>(std::ranges::distance(values)));
    for (const auto &value : values) {
        list.emplace_back(createDefaultValue(value));
    }
    return impl::Value::createValueList(std::move(list));
}

template <typename T>
auto createScalarMatrixValue(const std::vector<std::vector<T>> &values) -> impl::ValuePtr {
    std::vector<impl::ValuePtr> rows;
    rows.reserve(values.size());
    for (const auto &row : values) {
        rows.emplace_back(createScalarListValue(row));
    }
    return impl::Value::createValueList(std::move(rows));
}

}

auto createDefaultValue(const Integer value) -> impl::ValuePtr {
    return impl::Value::createInteger(value);
}

auto createDefaultValue(const bool value) -> impl::ValuePtr {
    return impl::Value::createBoolean(value);
}

auto createDefaultValue(const Float value) -> impl::ValuePtr {
    return impl::Value::createFloat(value);
}

auto createDefaultValue(const text::String &value) -> impl::ValuePtr {
    return impl::Value::createText(value);
}

auto createDefaultValue(const time::Date &value) -> impl::ValuePtr {
    return impl::Value::createDate(value);
}

auto createDefaultValue(const time::Time &value) -> impl::ValuePtr {
    return impl::Value::createTime(value);
}

auto createDefaultValue(const time::TimeWithZone &value) -> impl::ValuePtr {
    return impl::Value::createTimeWithZone(value);
}

auto createDefaultValue(const time::DateTime &value) -> impl::ValuePtr {
    return impl::Value::createDateTime(value);
}

auto createDefaultValue(const mem::ByteBlock &value) -> impl::ValuePtr {
    return impl::Value::createBytes(value);
}

auto createDefaultValue(const time::CalendarDelta &value) -> impl::ValuePtr {
    return impl::Value::createCalendarDelta(value);
}

auto createDefaultValue(const re::RegExPtr &value) -> impl::ValuePtr {
    if (value == nullptr) {
        throw err::ParameterError{"The regular expression cannot be null.", "value"};
    }
    return impl::Value::createRegEx(value);
}

auto createDefaultValue(const std::vector<Integer> &values) -> impl::ValuePtr {
    return createScalarListValue(values);
}

auto createDefaultValue(const std::vector<bool> &values) -> impl::ValuePtr {
    return createScalarListValue(values);
}

auto createDefaultValue(const std::vector<Float> &values) -> impl::ValuePtr {
    return createScalarListValue(values);
}

auto createDefaultValue(const text::StringList &values) -> impl::ValuePtr {
    return createScalarListValue(values);
}

auto createDefaultValue(const std::vector<mem::ByteBlock> &values) -> impl::ValuePtr {
    return createScalarListValue(values);
}

auto createDefaultValue(const std::vector<std::vector<Integer>> &values) -> impl::ValuePtr {
    return createScalarMatrixValue(values);
}

auto createDefaultValue(const std::vector<std::vector<Float>> &values) -> impl::ValuePtr {
    return createScalarMatrixValue(values);
}

}
