// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/Tool.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>

namespace tool::unicode {

using namespace el::text::literals;

/// Generate Unicode Light and normalization runtime data from one verified snapshot.
/// @tested{UnicodeDataToolTest}
class UnicodeDataTool final : public frame::Tool {
    /// Option key for --output-directory, overriding the generated Unicode-table directory.
    constexpr static auto cOutputOption = "output-directory"_el;
    /// Configuration key selecting the default generated Unicode-table directory.
    constexpr static auto cOutputDirectoryConfig = "output_directory"_el;
    /// Option key for --check, verifying generated Unicode tables without writing files.
    constexpr static auto cCheckOption = "check"_el;

public:
    // defaults
    UnicodeDataTool() = default;
    ~UnicodeDataTool() override = default;

public: // implement Tool
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    [[nodiscard]] auto usedDataSources() const -> el::StringList override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    el::Path _outputDirectory; ///< Destination for the three generated translation units.
    bool _checkOnly{};         ///< Check output without writing.
};

}
