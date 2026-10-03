// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"

#include <erbsland/ByteBlock.hpp>

#include <vector>

namespace tool::anti_patterns {

/// Immutable set of registered detectors used by every file worker.
class RuleRegistry final {
public:
    /// Construct all implemented detectors and verify unique identifiers.
    RuleRegistry();

public: // accessors
    /// Get detectors in stable registration order.
    [[nodiscard]] auto all() const noexcept -> const std::vector<RulePtr> & { return _rules; }
    /// Look up a detector by its stable identifier.
    [[nodiscard]] auto find(const el::String &identifier) const -> const Rule *;
    /// Get the local configuration digest used by the finding cache.
    [[nodiscard]] auto configurationSignature() const noexcept -> const el::ByteBlock & {
        return _configurationSignature;
    }

public: // configuration
    /// Register an optional section and its detector-owned settings for every rule.
    /// @param builder The tool's configuration schema builder.
    void addConfigurationRules(el::conf::vr::RulesBuilder &builder) const;
    /// Replace every detector's settings before any scan workers start.
    /// @param configuration The validated Anti Patterns section.
    void configure(const el::conf::ValuePtr &configuration);

private:
    std::vector<RulePtr> _rules;           ///< Detectors in stable registration order.
    el::ByteBlock _configurationSignature; ///< Digest of the complete configured anti-pattern subtree.
};

}
