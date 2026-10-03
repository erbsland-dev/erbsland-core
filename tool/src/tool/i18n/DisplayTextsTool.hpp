// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/Tool.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>

namespace tool::i18n {

using namespace el::text::literals;

/// Regenerate Core's checked-in display-text catalog.
/// @tested{DisplayTextsToolTest}
class DisplayTextsTool final : public frame::Tool {
    /// Option key for --check, verifying generated display texts without writing files.
    constexpr static auto cCheckOption = "check"_el;
    /// Configuration key selecting the display-text catalog input.
    constexpr static auto cInputFileConfig = "input_file"_el;
    /// Configuration key selecting the generated display-text header.
    constexpr static auto cOutputHeaderConfig = "output_header"_el;
    /// Configuration key selecting the generated display-text implementation.
    constexpr static auto cOutputSourceConfig = "output_source"_el;
    /// Configuration key naming the generated display-text class.
    constexpr static auto cClassNameConfig = "class_name"_el;
    /// Configuration key selecting the generated display-text namespace.
    constexpr static auto cNamespaceConfig = "namespace"_el;

public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    el::Path _input;         ///< Catalog input inside the project.
    el::Path _header;        ///< Generated header inside the project.
    el::Path _source;        ///< Generated implementation inside the project.
    el::String _className;   ///< Generated class identifier.
    el::String _namespace;   ///< Generated namespace.
    bool _checkOnly = false; ///< Compare without writing.
};

}
