// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Rule.hpp"
#include "../SourceBlock.hpp"

#include <erbsland/ItemIndex.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StringList.hpp>

#include <optional>

namespace tool::anti_patterns::rules {

using namespace el::text::literals;

/// Detect missing api documentation in one indexed C++ source.
class MissingApiDocumentationRule final : public Rule {
    /// A valid API scope with an optional containing class or struct.
    struct ApiScope {
        el::ItemIndex typeIndex = el::ItemIndex::noIndex(); ///< Containing class or struct in the source index.
    };

    /// Configuration key listing class-header text that exempts an entire file from API documentation checks.
    static constexpr auto cFileExemptionTextsConfig = "file_exemption_texts"_el;
    /// Configuration key listing declaration text that exempts individual declarations from API documentation checks.
    static constexpr auto cDeclarationExemptionTextsConfig = "declaration_exemption_texts"_el;

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
    /// Apply path and containing-type exemptions before scanning declarations.
    auto isFileExempt(const SourceIndex &source) const -> bool;
    /// Find an API scope and its nearest containing type, if any.
    /// @return No value inside a non-API block; otherwise the scope, with no type index for free declarations.
    static auto findApiScope(const SourceIndex &source, el::ByteIndex start) -> std::optional<ApiScope>;
    /// Recognize a constructor or destructor outside a documented defaults group.
    static auto isConstructor(
        const SourceIndex &source, const el::String &declarationText, const SourceBlock &type, el::LineIndex lineIndex)
        -> bool;
    /// Test declaration exclusions after recognizing an API candidate.
    auto isExempt(const SourceIndex &source, el::ByteIndex start, const el::String &statement) const -> bool;

private:
    el::StringList _fileExemptionTexts;        ///< Class-header substrings exempting a complete file.
    el::StringList _declarationExemptionTexts; ///< Declaration substrings exempting individual declarations.
};

}
