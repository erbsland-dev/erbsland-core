// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RuleRegistry.hpp"

#include "rules/AnonymousNamespaceRule.hpp"
#include "rules/ApiDocumentationOnMemberImplementationRule.hpp"
#include "rules/ErbslandCoreContainersStrictRule.hpp"
#include "rules/ForwardDeclarationRule.hpp"
#include "rules/ImplementationInWrongUnitRule.hpp"
#include "rules/MisplacedUnittestMetadataRule.hpp"
#include "rules/MissingApiDocumentationRule.hpp"
#include "rules/MissingDefaultGroupCommentRule.hpp"
#include "rules/MultipleNamespacesRule.hpp"
#include "rules/MultipleTypesInHeaderRule.hpp"
#include "rules/NamedConstantForRepeatedLiteralsRule.hpp"
#include "rules/NamespaceInWrongUnitRule.hpp"
#include "rules/NamespaceNoiseRule.hpp"
#include "rules/NestedNamespaceRule.hpp"
#include "rules/OversizedFileRule.hpp"
#include "rules/OversizedNestedTypeRule.hpp"
#include "rules/RegularStringLiteralRule.hpp"
#include "rules/StaticCastVoidRule.hpp"
#include "rules/StaticGlobalObjectRule.hpp"
#include "rules/StaticOnlyClassRule.hpp"
#include "rules/TypeInWrongUnitRule.hpp"
#include "rules/UnnecessaryTypeConstructionRule.hpp"
#include "rules/UnnecessaryUnitConversionRule.hpp"

#include <erbsland/conf/vr/RulesBuilderBranch.hpp>
#include <erbsland/HashAlgorithm.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/StringSet.hpp>

namespace tool::anti_patterns {

using namespace el::text::literals;

RuleRegistry::RuleRegistry() {
    _rules.emplace_back(rules::OversizedFileRule::create());
    _rules.emplace_back(rules::AnonymousNamespaceRule::create());
    _rules.emplace_back(rules::TypeInWrongUnitRule::create());
    _rules.emplace_back(rules::NamespaceInWrongUnitRule::create());
    _rules.emplace_back(rules::ImplementationInWrongUnitRule::create());
    _rules.emplace_back(rules::OversizedNestedTypeRule::create());
    _rules.emplace_back(rules::ForwardDeclarationRule::create());
    _rules.emplace_back(rules::StaticGlobalObjectRule::create());
    _rules.emplace_back(rules::StaticOnlyClassRule::create());
    _rules.emplace_back(rules::MissingApiDocumentationRule::create());
    _rules.emplace_back(rules::ApiDocumentationOnMemberImplementationRule::create());
    _rules.emplace_back(rules::MissingDefaultGroupCommentRule::create());
    _rules.emplace_back(rules::MultipleTypesInHeaderRule::create());
    _rules.emplace_back(rules::RegularStringLiteralRule::create());
    _rules.emplace_back(rules::MisplacedUnittestMetadataRule::create());
    _rules.emplace_back(rules::StaticCastVoidRule::create());
    _rules.emplace_back(rules::NestedNamespaceRule::create());
    _rules.emplace_back(rules::MultipleNamespacesRule::create());
    _rules.emplace_back(rules::ErbslandCoreContainersStrictRule::create());
    _rules.emplace_back(rules::NamedConstantForRepeatedLiteralsRule::create());
    _rules.emplace_back(rules::UnnecessaryUnitConversionRule::create());
    _rules.emplace_back(rules::UnnecessaryTypeConstructionRule::create());
    _rules.emplace_back(rules::NamespaceNoiseRule::create());
    auto identifiers = el::StringSet{};
    for (const auto &rule : _rules) {
        if (!rule || rule->identifier().isEmpty()) {
            throw el::LogicFailure{"An anti-pattern rule is missing its identifier."_el};
        }
        if (identifiers.contains(rule->identifier())) {
            throw el::LogicFailure{"Duplicate anti-pattern rule identifier."_el};
        }
        identifiers.insert(rule->identifier());
    }
}

auto RuleRegistry::find(const el::String &identifier) const -> const Rule * {
    for (const auto &rule : _rules) {
        if (rule->identifier() == identifier) {
            return rule.get();
        }
    }
    return nullptr;
}

void RuleRegistry::addConfigurationRules(el::conf::vr::RulesBuilder &builder) const {
    using namespace el::conf::vr::builder;
    for (const auto &rule : _rules) {
        const auto prefix = rule->identifier();
        builder.addRule(
            prefix,
            el::conf::vr::RuleType::Section,
            IsOptional{},
            Title{rule->title()},
            Description{rule->description()});
        auto branch = builder.createBranch(prefix);
        rule->addConfigurationRules(branch);
    }
}

void RuleRegistry::configure(const el::conf::ValuePtr &configuration) {
    for (const auto &rule : _rules) {
        const auto section = configuration->value(rule->identifier());
        rule->configure(section);
    }
    _configurationSignature = configuration->digest(el::cryptology::HashAlgorithm::Sha2_256);
}

}
