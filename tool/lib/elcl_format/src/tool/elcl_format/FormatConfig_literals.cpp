// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatConfig.hpp"

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/ParameterFailure.hpp>

namespace tool::elcl_format {

using namespace el::text::literals;
using namespace el::conf::syntax;
using el::conf::vr::RuleType;
using namespace el::conf::vr::builder;

void FormatConfig::addLiteralStyleRules(el::conf::vr::RulesBuilderBranch &branch) {
    branch.addRule("boolean"_el, RuleType::Section, IsOptional{}, Title{"Boolean formatting"_el});
    branch.addRule(cBooleanCase, RuleType::Text, IsOptional{}, Title{"Boolean case"_el});
    branch.addRule(cBooleanForm, RuleType::Text, IsOptional{}, Title{"Boolean word pair"_el});
}

void FormatConfig::readLiteralStyle(const el::conf::ValuePtr &section, FormatStylePatch &patch) {
    if (const auto value = section->value(cNamesAbbreviations)) {
        auto words = value->asStringListOrThrow();
        if (words.count() == el::ItemCount{1} && words.first().isEmpty()) {
            words.clear();
        }
        try {
            patch.setAbbreviations(std::move(words));
        } catch (const el::err::ParameterFailure &) {
            invalid(value);
        }
    }
    if (const auto value = section->value(cBooleanCase)) {
        patch.setBooleanCase(readWordCase(value));
    }
    if (const auto value = section->value(cBooleanForm)) {
        const auto text = normalized(value);
        if (text == "keep"_el) {
            patch.setBooleanForm(BooleanForm::Keep);
        } else if (text == "true_false"_el) {
            patch.setBooleanForm(BooleanForm::TrueFalse);
        } else if (text == "yes_no"_el) {
            patch.setBooleanForm(BooleanForm::YesNo);
        } else if (text == "enabled_disabled"_el) {
            patch.setBooleanForm(BooleanForm::EnabledDisabled);
        } else if (text == "on_off"_el) {
            patch.setBooleanForm(BooleanForm::OnOff);
        } else {
            invalid(value);
        }
    }
}

}
