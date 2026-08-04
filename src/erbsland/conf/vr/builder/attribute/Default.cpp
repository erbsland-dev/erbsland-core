// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Default.hpp"

#include "../../../../text/StringFormat.hpp"
#include "../../../impl/value/Value.hpp"
#include "../../../impl/vr/Rule.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

using namespace text::literals;

template <typename Range>
auto Default::createScalarListValue(const Range &values) -> impl::ValuePtr {
    std::vector<impl::ValuePtr> list;
    list.reserve(static_cast<std::size_t>(std::ranges::distance(values)));
    for (const auto &value : values) {
        list.emplace_back(impl::Value::createFromValue(value));
    }
    return impl::Value::createValueList(std::move(list));
}

template <typename T>
auto Default::createScalarMatrixValue(const std::vector<std::vector<T>> &values) -> impl::ValuePtr {
    std::vector<impl::ValuePtr> rows;
    rows.reserve(values.size());
    for (const auto &row : values) {
        rows.emplace_back(createScalarListValue(row));
    }
    return impl::Value::createValueList(std::move(rows));
}

Default::Default(impl::ValuePtr value) : _value{std::move(value)} {
}

Default::Default(const std::vector<Integer> &values) : _value{createScalarListValue(values)} {
}

Default::Default(const std::vector<bool> &values) : _value{createScalarListValue(values)} {
}

Default::Default(const std::vector<Float> &values) : _value{createScalarListValue(values)} {
}

Default::Default(const text::StringList &values) : _value{createScalarListValue(values)} {
}

Default::Default(const std::vector<mem::ByteBlock> &values) : _value{createScalarListValue(values)} {
}

Default::Default(const std::vector<std::vector<Integer>> &values) : _value{createScalarMatrixValue(values)} {
}

Default::Default(const std::vector<std::vector<Float>> &values) : _value{createScalarMatrixValue(values)} {
}

void Default::operator()(Rule &rule) {
    if (!rule.type().acceptsDefaults()) {
        throwValidationError(
            text::StringFormat{"A default value cannot be used for '{}' node rules"_el}.build(rule.type().toText()));
    }
    if (!rule.type().matchesValueType(_value->type())) {
        throwValidationError(
            text::StringFormat{
                "The default value of a node-rules definition must match its type. Expected {}, but got {}"_el}
                .build(rule.type().expectedValueTypeText(), _value->type().toValueDescription(true)));
    }
    rule.setDefaultValue(_value);
}

}
