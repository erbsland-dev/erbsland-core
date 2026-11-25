// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Rule.hpp"

namespace tool::anti_patterns::rules {

/// Require Core string containers for Core string elements and keys.
class ErbslandCoreContainersStrictRule final : public Rule {
public:
    /// Create this detector for the registry.
    [[nodiscard]] static auto create() -> RulePtr;

public: // implement Rule
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto severity() const -> Severity override;
    void scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const override;
};

}
