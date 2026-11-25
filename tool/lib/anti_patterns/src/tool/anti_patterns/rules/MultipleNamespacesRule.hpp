// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Rule.hpp"

namespace tool::anti_patterns::rules {

/// Detect distinct sibling namespace declarations in one source unit.
class MultipleNamespacesRule final : public Rule {
public:
    /// Create this detector for the registry.
    [[nodiscard]] static auto create() -> RulePtr;

public: // implement Rule
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto severity() const -> Severity override;
    void scan(const SourceIndex &source, el::List<el::ByteRange> &candidates) const override;

private:
    /// Test whether a std block contains only hash or formatter specialization definitions.
    /// @param source The indexed source.
    /// @param block The namespace block to inspect.
    /// @return Whether every declaration is an allowed standard-library extension.
    [[nodiscard]] static auto isStandardExtensionNamespace(const SourceIndex &source, const SourceBlock &block) -> bool;
};

}
