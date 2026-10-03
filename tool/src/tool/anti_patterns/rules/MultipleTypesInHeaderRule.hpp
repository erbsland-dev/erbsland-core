// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Rule.hpp"

namespace tool::anti_patterns::rules {

/// Detect multiple types in header in one indexed C++ source.
class MultipleTypesInHeaderRule final : public Rule {
public:
    /// Create this detector for the registry.
    [[nodiscard]] static auto create() -> RulePtr;

public: // implement Rule
    /// Get the stable rule identifier.
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    /// Get the report priority.
    [[nodiscard]] auto severity() const -> Severity override;
    /// Inspect one source index for this rule.
    void scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const override;
};

}
