// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NamespaceNoiseRule.hpp"

#include "../impl/TokenView.hpp"

#include <erbsland/StringJoins.hpp>
#include <erbsland/StringList.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

auto NamespaceNoiseRule::create() -> RulePtr {
    return std::make_unique<NamespaceNoiseRule>();
}

auto NamespaceNoiseRule::identifier() const -> el::String {
    return "namespace_noise"_el;
}

auto NamespaceNoiseRule::title() const -> el::String {
    return "Redundant namespace qualifiers"_el;
}

auto NamespaceNoiseRule::description() const -> el::String {
    return "Detect repeated namespace qualifiers that obscure code within a domain."_el;
}

auto NamespaceNoiseRule::severity() const -> Severity {
    return Severity::Low;
}

void NamespaceNoiseRule::addConfigurationRules(el::conf::vr::RulesBuilder &builder) const {
    using el::conf::vr::RuleType;
    using namespace el::conf::vr::builder;
    builder.addRule(
        cDomainsConfig,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        EntryConstraints{Matches{R"(\A[A-Za-z_][A-Za-z0-9_]*\z)"_el}},
        IsOptional{},
        Title{"Domain namespaces"_el},
        Description{"Case-sensitive domain identifiers counted independently in each implementation namespace."_el});
    builder.addRule(
        cIgnoredSymbolsConfig,
        RuleType::ValueList,
        EntryType{RuleType::Text},
        EntryConstraints{Matches{R"(\A[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)+\z)"_el}},
        IsOptional{},
        Title{"Ignored qualified symbols"_el},
        Description{"Complete domain-qualified symbols that do not contribute to namespace-noise counts."_el});
}

void NamespaceNoiseRule::configure(const el::conf::ValuePtr &configuration) {
    if (configuration == nullptr) {
        _domains.clear();
        _ignoredSymbols.clear();
        return;
    }
    _domains = configuration->getStringList(cDomainsConfig);
    _ignoredSymbols = configuration->getStringList(cIgnoredSymbolsConfig);
}

auto NamespaceNoiseRule::isIgnoredSymbol(const impl::TokenView &tokens, el::ItemIndex index) const -> bool {
    auto parts = el::StringList{tokens.at(index).text()};
    while (tokens.at(index.incremented()).text() == "::"_el) {
        index.advance(el::ItemCount{2});
        if (tokens.at(index).kind() != el::cpp::CppToken::Kind::Identifier) {
            return false;
        }
        parts.append(tokens.at(index).text());
    }
    return _ignoredSymbols.contains(parts.join("::"_el));
}

void NamespaceNoiseRule::scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const {
    if (!source.path().toString().endsWith(".cpp"_el)) {
        return;
    }
    const auto tokens = impl::TokenView{source};
    for (const auto &block : source.blocks()) {
        if (block.kind != SourceBlock::Kind::Namespace || block.name.isEmpty()) {
            continue;
        }
        for (const auto &domain : _domains) {
            auto count = el::ItemCount{};
            auto first = el::ByteRange::noRange();
            auto declaration = false;
            for (auto index = el::ItemIndex::zero(); index.isWithin(tokens.count()); ++index) {
                const auto &token = tokens.at(index);
                const auto &word = token.text();
                if (word == ";"_el || word == "{"_el || word == "}"_el) {
                    declaration = false;
                } else if (word == "using"_el || word == "namespace"_el) {
                    declaration = true;
                }
                if (declaration || word != domain || tokens.at(index.incremented()).text() != "::"_el ||
                    token.position() <= block.openingBrace() || token.position() >= block.closingBrace() ||
                    source.insideOsConditional(token.position())) {
                    continue;
                }
                if (!index.isZero() && tokens.at(index.decremented()).text() == "::"_el) {
                    if (index < el::ItemIndex{2}) {
                        continue;
                    }
                    const auto prefix = index.retreated(el::ItemCount{2});
                    if ((tokens.at(prefix).text() != "el"_el && tokens.at(prefix).text() != "erbsland"_el) ||
                        (!prefix.isZero() && tokens.at(prefix.decremented()).text() == "::"_el)) {
                        continue;
                    }
                }
                auto owner = source.enclosingBlock(token.position());
                while (
                    owner.isWithin(source.blocks().count()) &&
                    source.blocks().getRef(owner).kind != SourceBlock::Kind::Namespace) {
                    owner = source.blocks().getRef(owner).parent;
                }
                if (!owner.isWithin(source.blocks().count()) || &source.blocks().getRef(owner) != &block) {
                    continue;
                }
                if (isIgnoredSymbol(tokens, index)) {
                    continue;
                }
                if (count.isZero()) {
                    first = {token.position(), tokens.at(index.incremented()).range().endIndex()};
                }
                ++count;
            }
            if (count > el::ItemCount{3}) {
                candidates.append(first);
            }
        }
    }
}

}
