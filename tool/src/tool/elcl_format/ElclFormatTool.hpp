// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/Tool.hpp"

#include <erbsland/conf/syntax/Format.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>

namespace tool::elcl_format {

using namespace el::text::literals;

/// Reformat local ELCL files independently of an Erbsland Core checkout.
class ElclFormatTool final : public frame::Tool {
    /// Positional option key selecting the ELCL files or directories to format.
    static constexpr auto cPaths = "paths"_el;
    /// Option key for --output-only, printing one formatted file to stdout without changing it.
    static constexpr auto cOutputOnly = "output_only"_el;
    /// Option key for --recursive, discovering ELCL files in nested directories.
    static constexpr auto cRecursive = "recursive"_el;

public: // implement Tool
    auto identifier() const -> el::String override;
    auto title() const -> el::String override;
    auto description() const -> el::String override;
    auto optionModule() const -> el::OptionModulePtr override;
    auto requiresProject() const noexcept -> bool override { return false; }
    void configure(const el::conf::ValuePtr &, const el::OptionValuesPtr &values) override;
    void execute() override;

private:
    /// Discover and deduplicate requested input files.
    void discover();
    /// Find the nearest configuration or explicit override.
    auto findConfiguration(const el::Path &file) const -> el::Path;
    /// Format one file and replace it only when its bytes change.
    void formatFile(const el::Path &file, const el::conf::syntax::Format &format);
    /// Write a diagnostic to stderr and record errors.
    void report(const el::Path &file, const el::String &message, bool error = true);

private:
    el::OptionValuesPtr _options; ///< Parsed options retained for discovery diagnostics.
    el::StringList _requested;    ///< Explicit files and directories.
    std::vector<el::Path> _files; ///< Deduplicated sorted input files.
    el::Path _configuration;      ///< Explicit configuration override.
    bool _recursive{};            ///< Scan descendants.
    bool _outputOnly{};           ///< Print one result without replacing it.
    bool _failed{};               ///< Whether any per-file operation failed.
};

}
