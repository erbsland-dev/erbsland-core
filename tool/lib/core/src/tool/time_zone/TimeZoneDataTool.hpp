// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::time_zone {

using namespace el::text::literals;

/// Generate all compact time-zone runtime tables from the verified tzdb and CLDR snapshots.
/// @tested{TimeZoneDataToolTest}
class TimeZoneDataTool final : public frame::Tool {
    /// Option key for --output-directory, overriding the generated time-zone data directory.
    constexpr static auto cOutputDirectoryOption = "output-directory"_el;
    /// Configuration key selecting the default generated time-zone data directory.
    constexpr static auto cOutputDirectoryConfig = "output_directory"_el;
    /// Option key for --check, verifying generated time-zone data without writing files.
    constexpr static auto cCheckOption = "check"_el;

public:
    [[nodiscard]] auto identifier() const -> el::String override;
    [[nodiscard]] auto title() const -> el::String override;
    [[nodiscard]] auto description() const -> el::String override;
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    [[nodiscard]] auto usedDataSources() const -> el::StringList override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    el::Path _outputDirectory; ///< Checked generated-file directory.
    bool _checkOnly{};         ///< Check without writing.
};

}
