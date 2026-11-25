// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CleanupPolicy.hpp"

#include <erbsland/Literals.hpp>
#include <tool/frame/PreCommitTask.hpp>
#include <tool/frame/Tool.hpp>

namespace tool::cleanup {

using namespace el::text::literals;

/// Combined include, source-layout, CMake and incremental clang-format maintenance command.
class CodeCleanupTool final : public frame::Tool, public frame::PreCommitTask {
    /// Command identifier shared by registration and the cleanup configuration section.
    constexpr static auto cIdentifier = "code_cleanup"_el;
    /// Option key for --check, reporting required cleanup without writing files.
    constexpr static auto cCheckOption = "check"_el;
    /// Option key for --full-format, refreshing the formatting baseline for every selected file.
    constexpr static auto cFullFormatOption = "full_format"_el;
    /// Option key for --jobs, limiting parallel cleanup workers.
    constexpr static auto cJobsOption = "jobs"_el;
    /// Long command-line spelling for read-only cleanup validation.
    constexpr static auto cCheckFlag = "--check"_el;
    /// Long command-line spelling for a complete formatting-baseline refresh.
    constexpr static auto cFullFormatFlag = "--full-format"_el;
    /// Long command-line spelling for the cleanup worker count.
    constexpr static auto cJobsFlag = "--jobs"_el;
    /// Short command-line spelling for the cleanup worker count.
    constexpr static auto cJobsShortFlag = "-j"_el;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    CleanupPolicy _policy;     ///< Validated settings and resolved input roots.
    unsigned int _workers = 1; ///< Bounded parallel worker count.
    bool _checkOnly = false;   ///< Read-only validation.
    bool _fullFormat = false;  ///< Explicit baseline refresh.
};

}
