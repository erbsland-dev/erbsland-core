// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamedConstantForRepeatedLiteralsRule.hpp"

#include <erbsland/StringJoins.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSet.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto NamedConstantForRepeatedLiteralsRule::create() -> RulePtr {
    return std::make_unique<NamedConstantForRepeatedLiteralsRule>();
}

auto NamedConstantForRepeatedLiteralsRule::identifier() const -> el::String {
    return "named_constant_for_repeated_literals"_el;
}

auto NamedConstantForRepeatedLiteralsRule::title() const -> el::String {
    return "Repeated configuration and option keys"_el;
}

auto NamedConstantForRepeatedLiteralsRule::description() const -> el::String {
    return "Detect repeated key literals that should share a named constant."_el;
}

auto NamedConstantForRepeatedLiteralsRule::severity() const -> Severity {
    return Severity::Low;
}

void NamedConstantForRepeatedLiteralsRule::addConfigurationRules(el::conf::vr::RulesBuilder &builder) const {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    builder.addRule(
        cKeyFunctionsConfig,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        EntryConstraints{Matches{R"(\A[A-Za-z_][A-Za-z0-9_]*\z)"_el}},
        IsOptional{},
        Title{"Key lookup functions"_el},
        Description{"Exact case-sensitive function names whose first argument is a configuration or option key."_el});
    builder.addRule(
        cKeyFunctionPrefixesConfig,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        EntryConstraints{Matches{R"(\A[A-Za-z_][A-Za-z0-9_]*\z)"_el}},
        IsOptional{},
        Title{"Key lookup function prefixes"_el},
        Description{"Case-sensitive function-name prefixes whose first argument is a configuration or option key."_el});
}

void NamedConstantForRepeatedLiteralsRule::configure(const el::conf::ValuePtr &configuration) {
    if (configuration == nullptr) {
        _keyFunctions.clear();
        _keyFunctionPrefixes.clear();
        return;
    }
    _keyFunctions = configuration->getStringList(cKeyFunctionsConfig);
    _keyFunctionPrefixes = configuration->getStringList(cKeyFunctionPrefixesConfig);
}

auto NamedConstantForRepeatedLiteralsRule::isKeyArgument(
    const SourceIndex &source, const el::ItemIndex tokenIndex) const -> bool {
    const auto &tokens = source.tokens();
    auto previous = tokenIndex;
    while (!previous.isZero()) {
        --previous;
        if (tokens.getRef(previous).kind() == el::cpp::CppToken::Kind::Comment ||
            tokens.getRef(previous).kind() == el::cpp::CppToken::Kind::Directive) {
            continue;
        }
        auto value = tokens.getRef(previous).text();
        if (value == "{"_el && !previous.isZero()) {
            --previous;
            value = tokens.getRef(previous).text();
        }
        if (value != "("_el || previous.isZero()) {
            return false;
        }
        const auto name = tokens.getRef(previous.decremented()).text();
        if (_keyFunctions.contains(name)) {
            return true;
        }
        for (const auto &prefix : _keyFunctionPrefixes) {
            if (name.startsWith(prefix)) {
                return true;
            }
        }
        return false;
    }
    return false;
}

void NamedConstantForRepeatedLiteralsRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (source.path().suffix() != ".cpp"_el) {
        return;
    }
    auto firstByLiteral = el::StringMap<el::ByteRange>{};
    auto reported = el::StringSet{};
    for (const auto tokenIndex : source.literals()) {
        const auto &token = source.tokens().getRef(tokenIndex);
        const auto literal = token.text();
        if (!literal.startsWith("\""_el) || literal.startsWith("\"\""_el) || !isKeyArgument(source, tokenIndex)) {
            continue;
        }
        const auto end = token.range().endIndex();
        const auto suffix = source.slice(el::ByteRange{end, end + el::ByteCount{3}});
        if (!literal.endsWith("_el"_el) && suffix != "_el"_el) {
            continue;
        }
        const auto range = token.range();
        if (firstByLiteral.contains(literal)) {
            if (!reported.contains(literal)) {
                candidates.append(range);
                reported.insert(literal);
            }
        } else {
            firstByLiteral.set(literal, range);
        }
    }
}

}
