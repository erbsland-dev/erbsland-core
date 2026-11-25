// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatConfig.hpp"

#include <erbsland/all_conf_vr.hpp>
#include <erbsland/all_conf_vr_builder.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/syntax/Formatter.hpp>
#include <erbsland/conf/syntax/Parser.hpp>
#include <erbsland/ParseError.hpp>

#include <array>
#include <utility>

namespace tool::elcl_format {

using namespace el::text::literals;
using namespace el::conf::syntax;
using el::conf::vr::RuleType;
using namespace el::conf::vr::builder;
auto FormatConfig::normalized(const el::conf::ValuePtr &value) -> el::String {
    return el::conf::Name::createRegular(value->asTextOrThrow()).asText();
}

void FormatConfig::invalid(const el::conf::ValuePtr &value) {
    throw el::conf::ConfError{value, "Invalid ELCL Format"_el, "Unsupported formatting setting or selector value."_el};
}

auto FormatConfig::readValueType(const el::String &name, const el::conf::ValuePtr &value) -> el::conf::ValueType {
    static constexpr auto cSupportedTypes = std::array{
        std::pair{"integer"_el, el::conf::ValueType::Integer},
        std::pair{"boolean"_el, el::conf::ValueType::Boolean},
        std::pair{"float"_el, el::conf::ValueType::Float},
        std::pair{"text"_el, el::conf::ValueType::Text},
        std::pair{"date"_el, el::conf::ValueType::Date},
        std::pair{"time"_el, el::conf::ValueType::Time},
        std::pair{"date_time"_el, el::conf::ValueType::DateTime},
        std::pair{"bytes"_el, el::conf::ValueType::Bytes},
        std::pair{"time_delta"_el, el::conf::ValueType::TimeDelta},
        std::pair{"regex"_el, el::conf::ValueType::RegEx},
        std::pair{"value_list"_el, el::conf::ValueType::ValueList},
    };
    const auto normalizedName = el::conf::Name::createRegular(name).asText();
    for (const auto &[spelling, type] : cSupportedTypes) {
        if (normalizedName == spelling) {
            return type;
        }
    }
    invalid(value);
}

auto FormatConfig::readSelector(const el::conf::ValuePtr &section) -> Selector {
    auto selector = Selector{};
    if (!section) {
        return selector;
    }
    if (auto value = section->value(cKind)) {
        const auto kind = normalized(value);
        if (kind == cSection) {
            selector.setKind(Statement::Kind::Section);
        } else if (kind == "assignment"_el) {
            selector.setKind(Statement::Kind::Assignment);
        } else {
            invalid(value);
        }
    }
    if (auto value = section->value(cListSection)) {
        selector.setListSection(value->asBooleanOrThrow());
    }
    if (auto value = section->value(cRelativeSection)) {
        selector.setRelativeSection(value->asBooleanOrThrow());
    }
    if (auto value = section->value(cSectionNamesMoreThan)) {
        selector.setMinimumSectionNames(static_cast<std::size_t>(value->asIntegerOrThrow()));
    }
    if (auto value = section->value(cMetaValue)) {
        selector.setMetaValue(value->asBooleanOrThrow());
    }
    for (const auto &pattern : section->getStringList(cPaths)) {
        try {
            selector.addPath(pattern);
        } catch (const el::ParseError &) {
            throw el::conf::ConfError{
                section->valueOrThrow(cPaths),
                "Invalid Formatting Selector"_el,
                "Expected a valid name-path glob."_el,
                std::current_exception()};
        }
    }
    auto types = std::vector<el::conf::ValueType>{};
    for (const auto &name : section->getStringList(cValueTypes)) {
        types.emplace_back(readValueType(name, section->value(cValueTypes)));
    }
    selector.setValueTypes(std::move(types));
    return selector;
}

auto FormatConfig::read() const -> Format {
    auto document = el::conf::Parser{}.parseFileOrThrow(_path);
    auto rules = el::conf::vr::RulesBuilder{};
    rules.configureRoot(
        Title{"Formatter configuration"_el},
        Description{"Base style and conditional formatting rules for ELCL documents."_el});
    rules.addRule(
        "elcl_format"_el,
        RuleType::Section,
        Title{"ELCL formatting"_el},
        Description{"Base formatting style and ordered conditional overrides."_el});
    rules.addRule(
        cElclFormatBase,
        RuleType::Section,
        IsOptional{},
        Title{"Base style"_el},
        Description{"Formatting settings applied before conditional style rules."_el});
    addStyleRules(rules.createBranch(cElclFormatBase));
    rules.addRule(
        cElclFormatRules,
        RuleType::SectionList,
        IsOptional{},
        Title{"Style rules"_el},
        Description{"Ordered selectors and style overrides applied to matching syntax nodes."_el});
    rules.addRule(
        cElclFormatRulesVrEntry,
        RuleType::Section,
        Title{"Style rule"_el},
        Description{"One selector and the style changes for matching syntax nodes."_el});
    auto entry = rules.createBranch(cElclFormatRulesVrEntry);
    entry.addRule(
        cSelector,
        RuleType::Section,
        IsOptional{},
        Title{"Node selector"_el},
        Description{"Conditions that must match before applying this style."_el});
    entry.addRule(
        cStyle,
        RuleType::Section,
        Title{"Style override"_el},
        Description{"Formatting settings applied to nodes matching this rule."_el});
    addStyleRules(entry.createBranch(cStyle));
    auto selector = entry.createBranch(cSelector);
    selector.addRule(
        cKind,
        RuleType::Text,
        IsOptional{},
        Title{"Node kind"_el},
        Description{"Restrict the rule to section or value nodes."_el});
    selector.addRule(
        cListSection,
        RuleType::Boolean,
        IsOptional{},
        Title{"List section selector"_el},
        Description{"Require or exclude list sections."_el});
    selector.addRule(
        cRelativeSection,
        RuleType::Boolean,
        IsOptional{},
        Title{"Relative section selector"_el},
        Description{"Require or exclude relative section names."_el});
    selector.addRule(
        cMetaValue,
        RuleType::Boolean,
        IsOptional{},
        Title{"Metadata selector"_el},
        Description{"Require or exclude metadata values."_el});
    selector.addRule(
        cSectionNamesMoreThan,
        RuleType::Integer,
        IsOptional{},
        Minimum{0},
        Title{"Section name threshold"_el},
        Description{"Match sections with more than this number of name components."_el});
    selector.addRule(
        cPaths,
        RuleType::Alternatives,
        IsOptional{},
        Title{"Name path patterns"_el},
        Description{"One name-path glob or a list of globs selecting matching nodes."_el});
    selector.addAlternative(
        cPaths, RuleType::Text, Title{"Single name path pattern"_el}, Description{"Match one name-path glob."_el});
    selector.addAlternative(
        cPaths,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        Title{"Name path pattern list"_el},
        Description{"Match any of these name-path globs."_el});
    selector.addRule(
        cValueTypes,
        RuleType::Alternatives,
        IsOptional{},
        Title{"Value types"_el},
        Description{"One value type or a list of types selecting matching values."_el});
    selector.addAlternative(
        cValueTypes, RuleType::Text, Title{"Single value type"_el}, Description{"Match one ELCL value type."_el});
    selector.addAlternative(
        cValueTypes,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        Title{"Value type list"_el},
        Description{"Match any of these ELCL value types."_el});
    rules.takeRules()->validate(document, 1);
    auto result = Format{};
    auto style = result.baseStyle();
    readStyle(document->value(cElclFormatBase)).applyTo(style);
    result.setBaseStyle(std::move(style));
    if (auto entries = document->value(cElclFormatRules)) {
        for (const auto &rule : *entries) {
            result.addRule({readSelector(rule->value(cSelector)), readStyle(rule->value(cStyle))});
        }
    }
    // Validate all global counts even for an empty input selection.
    [[maybe_unused]] const auto validated = Formatter{result}.format(el::conf::syntax::Parser{}.parseText({}));
    return result;
}

}
