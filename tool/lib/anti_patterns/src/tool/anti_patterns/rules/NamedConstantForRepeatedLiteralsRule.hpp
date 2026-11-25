// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Rule.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/StringList.hpp>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

/// Find repeated Core string literals used as configuration or option keys.
class NamedConstantForRepeatedLiteralsRule final : public Rule {
    /// Configuration key listing exact function names whose first argument is a lookup key.
    static constexpr auto cKeyFunctionsConfig = "key_functions"_el;
    /// Configuration key listing function-name prefixes whose first argument is a lookup key.
    static constexpr auto cKeyFunctionPrefixesConfig = "key_function_prefixes"_el;

public:
    /// Create this detector for the registry.
    [[nodiscard]] static auto create() -> RulePtr;

public: // implement Rule
    void addConfigurationRules(el::conf::vr::RulesBuilder &builder) const override;
    void configure(const el::conf::ValuePtr &configuration) override;
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto severity() const -> Severity override;
    void scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const override;

private:
    /// Recognize the first literal argument of a key-oriented call.
    [[nodiscard]] auto isKeyArgument(const SourceIndex &source, el::ItemIndex tokenIndex) const -> bool;

private:
    el::StringList _keyFunctions;        ///< Exact function names whose first argument is a key.
    el::StringList _keyFunctionPrefixes; ///< Function-name prefixes whose first argument is a key.
};

}
