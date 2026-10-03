// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Rule.hpp"

namespace tool::anti_patterns::rules {

/// Detect regular string literal in one indexed C++ source.
class RegularStringLiteralRule final : public Rule {
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

private:
    /// Find adjacent literal tokens separated only by whitespace.
    auto adjacentGroupEnd(const SourceIndex &source, el::ItemIndex index) const -> el::ItemIndex;
    /// Test whether the preceding text explicitly selects a constructor or macro argument context.
    static auto isExplicitContext(const el::String &before) -> bool;
    /// Test whether a literal directly supplies the parameter name of a parameter-failure call.
    [[nodiscard]] auto isParameterNameArgument(const SourceIndex &source, el::ItemIndex tokenIndex) const -> bool;
    /// Get a literal suffix immediately following a token.
    [[nodiscard]] auto suffix(const SourceIndex &source, const el::cpp::CppToken &token) const -> el::String;
    /// Get the byte after a literal and its suffix.
    [[nodiscard]] auto literalEnd(const SourceIndex &source, const el::cpp::CppToken &token) const -> el::ByteIndex;
};

}
