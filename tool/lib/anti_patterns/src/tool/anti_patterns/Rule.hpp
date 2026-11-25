// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Finding.hpp"
#include "SourceIndex.hpp"

#include <erbsland/ByteRange.hpp>
#include <erbsland/conf/Value.hpp>
#include <erbsland/conf/vr/RulesBuilder.hpp>
#include <erbsland/List.hpp>
#include <erbsland/String.hpp>

#include <memory>

namespace tool::anti_patterns {

/// An independently registered detector with immutable settings during scanning.
class Rule {
public:
    virtual ~Rule() = default;

public: // detection
    /// Return the stable identifier used by configuration and documentation.
    [[nodiscard]] virtual auto identifier() const -> el::String = 0;
    /// Return the human-readable name used in configuration help.
    [[nodiscard]] virtual auto title() const -> el::String = 0;
    /// Explain the code-quality issue detected by this rule.
    [[nodiscard]] virtual auto description() const -> el::String = 0;
    /// Return the report priority of this rule.
    [[nodiscard]] virtual auto severity() const -> Severity = 0;
    /// Add source locations detected by this rule.
    virtual void scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const = 0;

public: // configuration
    /// Add settings below this detector's optional section.
    /// @param builder The tool's configuration schema builder.
    virtual void addConfigurationRules([[maybe_unused]] el::conf::vr::RulesBuilder &builder) const {}
    /// Replace settings before scanning; a null section resets all settings.
    /// @param configuration The validated optional rule section.
    virtual void configure([[maybe_unused]] const el::conf::ValuePtr &configuration) {}
};

using RulePtr = std::unique_ptr<Rule>;

}
