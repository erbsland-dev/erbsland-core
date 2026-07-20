// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Default.hpp"

#include "../../../../text/StringFormat.hpp"
#include "../../../impl/value/Value.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void Default::operator()(impl::Rule &rule) {
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
