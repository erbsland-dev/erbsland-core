// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BoxFrameData.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/render/Value.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::cterm {

using namespace el::text::literals;

/// Shared command behavior for the two source-backed terminal frame generators.
/// @tested{BoxFrameToolTest}
class FrameGeneratorTool : public frame::Tool {
    /// Option key for --output-file, overriding the generated frame-data destination.
    constexpr static auto cOutputFileOption = "output-file"_el;
    /// Configuration key selecting the default generated frame-data destination.
    constexpr static auto cOutputFileConfig = "output_file"_el;
    /// Option key for --check, verifying generated frame data without writing files.
    constexpr static auto cCheckOption = "check"_el;

public:
    /// Configure the default generated destination.
    /// @param defaultOutput Project-relative default output file.
    explicit FrameGeneratorTool(el::String defaultOutput);

public: // implement Tool
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    [[nodiscard]] auto usedDataSources() const -> el::StringList override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

protected: // rendering
    /// Render a complete generated C++ file from verified frame data.
    [[nodiscard]] virtual auto render(const BoxFrameData &data) const -> el::String = 0;
    /// Prepare one numeric array for a render template.
    [[nodiscard]] static auto arrayContext(
        const el::String &name, const el::String &type, const std::vector<uint32_t> &values, uint32_t width)
        -> el::text::render::ValueMap;

private:
    el::String _defaultOutput; ///< Configured default destination.
    el::Path _outputFile;      ///< Checked output path.
    bool _checkOnly{};         ///< Check output without writing.
};

}
