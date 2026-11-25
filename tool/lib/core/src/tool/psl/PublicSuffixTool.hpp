// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "EncodedData.hpp"
#include "PublicSuffixList.hpp"

#include <erbsland/Literals.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::psl {

using namespace el::text::literals;

/// Generate the library's compact PSL payload using independent Unicode sources.
/// @tested{PublicSuffixTest PublicSuffixFullTest}
class PublicSuffixTool final : public frame::Tool {
    /// Option key for --output-file, overriding the generated public-suffix destination.
    constexpr static auto cOutputFileOption = "output-file"_el;
    /// Option key for --check, verifying generated public-suffix data without writing files.
    constexpr static auto cCheckOption = "check"_el;
    /// Configuration key selecting the default generated public-suffix destination.
    constexpr static auto cOutputFileConfig = "output_file"_el;

public:
    // defaults
    PublicSuffixTool() = default;
    ~PublicSuffixTool() override = default;

public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    [[nodiscard]] auto usedDataSources() const -> el::StringList override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

public: // rendering
    /// Render the complete generated translation unit using compiled layouts.
    /// @param version Safe snapshot version identifier.
    /// @param data Derived runtime data.
    /// @return The generated C++ source.
    /// @throws el::ParseError If the encoded data fails verification.
    /// @throws el::render::RenderError If the resource or template cannot be rendered.
    [[nodiscard]] static auto render(const el::String &version, const EncodedData &data) -> el::String;

private:
    /// Format a complete named C++ array for template insertion.
    /// @param type The element type name.
    /// @param name The generated array identifier.
    /// @param values Values to render in their existing order.
    /// @param hexadecimal Whether to use hexadecimal rather than decimal literals.
    /// @return The formatted array declaration.
    [[nodiscard]] static auto renderArray(
        const el::String &type, const el::String &name, const el::List<unsigned> &values, bool hexadecimal)
        -> el::String;
    /// Report a generated-file mismatch in check mode.
    [[noreturn]] void throwOutputMismatch() const;

private:
    el::Path _outputFile;    ///< The C++ output file path.
    bool _checkOnly = false; ///< Flag if this is a check-only run (skips the actual generation).
};

}
