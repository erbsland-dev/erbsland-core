// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IdnaData.hpp"

#include "../frame/Tool.hpp"

#include <erbsland/Literals.hpp>

namespace tool::idna {

using namespace el::text::literals;

/// Generate the library's compact IDNA2008 lookup tables from checked-in UCD sources.
/// @tested{IdnaToolTest IdnaDataFullTest}
class IdnaTool final : public frame::Tool {
    /// Option key for --output-file, overriding the generated IDNA-table destination.
    constexpr static auto cOutputFileOption = "output-file"_el;
    /// Option key for --check, verifying generated IDNA data without writing files.
    constexpr static auto cCheckOption = "check"_el;
    /// Configuration key selecting the default generated IDNA-table destination.
    constexpr static auto cOutputFileConfig = "output_file"_el;

public:
    // defaults
    IdnaTool() = default;
    ~IdnaTool() override = default;

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
    /// @param data Derived runtime data.
    /// @return The generated C++ source.
    /// @throws el::render::RenderError If the resource or template cannot be rendered.
    [[nodiscard]] static auto render(const IdnaData &data) -> el::String;

private:
    /// Report a generated-file mismatch in check mode.
    [[noreturn]] void throwOutputMismatch() const;

private:
    el::Path _outputFile;    ///< The C++ output file path.
    bool _checkOnly = false; ///< Flag if this is a check-only run (skips the actual generation).
};

}
