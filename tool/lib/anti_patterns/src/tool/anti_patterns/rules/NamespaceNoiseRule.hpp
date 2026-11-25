// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../impl/TokenView.hpp"
#include "../Rule.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/StringList.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

/// Detect repeated domain qualifications in implementation namespace scopes.
class NamespaceNoiseRule final : public Rule {
    /// Configuration key listing domain prefixes counted as redundant qualifications.
    static constexpr auto cDomainsConfig = "domains"_el;
    /// Configuration key listing qualified symbols excluded from namespace-noise detection.
    static constexpr auto cIgnoredSymbolsConfig = "ignored_symbols"_el;

public:
    /// Create this detector for the registry.
    [[nodiscard]] static auto create() -> RulePtr;

public: // implement Rule
    void addConfigurationRules(el::conf::vr::RulesBuilder &builder) const override;
    void configure(const el::conf::ValuePtr &configuration) override;
    /// Get the stable rule identifier.
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    /// Get the report priority.
    [[nodiscard]] auto severity() const -> Severity override;
    /// Inspect one source index for this rule.
    void scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const override;

private:
    /// Test a complete qualified identifier beginning with a counted domain.
    [[nodiscard]] auto isIgnoredSymbol(const impl::TokenView &tokens, el::ItemIndex index) const -> bool;

private:
    el::StringList _domains;        ///< Domain prefixes eligible for detection.
    el::StringList _ignoredSymbols; ///< Complete domain-qualified identifiers excluded from counting.
};

}
