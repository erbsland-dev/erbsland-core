// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/DemoSynchronizer.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::doc {

using namespace el::text::literals;

/// Clean RST and optionally synchronize managed demo excerpts and captures.
class ProcessRstTool final : public frame::Tool {
    /// Positional option key selecting the RST file or directory to process.
    static constexpr auto cTarget = "target"_el;
    /// Option key for --recursive, including RST files in nested directories.
    static constexpr auto cRecursive = "recursive"_el;
    /// Option key for --force, regenerating demo blocks even when fingerprints match.
    static constexpr auto cForce = "force"_el;
    /// Option key for --check, failing on proposed document changes without writing.
    static constexpr auto cCheck = "check"_el;
    /// Option key for --dry-run, reporting proposed document changes without writing.
    static constexpr auto cDryRun = "dry-run"_el;
    /// Option key for --demos, selecting demo validation, synchronization, refresh, or migration.
    static constexpr auto cDemos = "demos"_el;
    /// Option key for --no-cleanup, skipping RST normalization during demo processing.
    static constexpr auto cNoCleanup = "no-cleanup"_el;
    /// Configuration key limiting the width of reflowed RST paragraphs.
    static constexpr auto cLineWidth = "line_width"_el;
    /// Configuration key selecting the Doxygen XML index used to resolve API links.
    static constexpr auto cDoxygenIndex = "doxygen_index"_el;
    /// Configuration key selecting the demo executable directory.
    static constexpr auto cDemoExecutableRoot = "demo_executable_root"_el;
    /// Configuration section list containing source presentation rules.
    static constexpr auto cDemoTextReplacement = "demo_text_replacement"_el;
    /// Configuration keys within each replacement rule.
    static constexpr auto cReplacementPattern = "pattern"_el;
    static constexpr auto cReplacementText = "replacement"_el;

public:
    // defaults
    ProcessRstTool() = default;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &) override;
    void execute() override;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    impl::DemoSynchronizer::TextReplacements _demoTextReplacements; ///< Ordered source presentation rules.
    el::Path _target;                                               ///< Selected document or directory.
    el::Path _demoExecutableRoot;                                   ///< Configured demo executable directory.
    el::Path _doxygenIndex;                                         ///< Public XML symbol index.
    el::String _demos;                                              ///< Optional demo processing mode.
    int _lineWidth{120};                                            ///< Simple paragraph width.
    bool _recursive{false};                                         ///< Include nested directories.
    bool _force{false};  ///< Regenerate demo blocks regardless of fingerprints.
    bool _check{false};  ///< Fail on proposed changes without writing.
    bool _dryRun{false}; ///< Report proposed changes without writing.
    bool _cleanup{true}; ///< Normalize RST after demo processing.
};

}
