// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../frame/Tool.hpp"

#include <erbsland/OptionValue.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/render/Environment.hpp>
#include <erbsland/render/Value.hpp>
#include <erbsland/StringMap.hpp>

#include <exception>

namespace tool::test_data {

using namespace el::text::literals;

/// Share validated destinations, text templates, and deferred fixture publication.
class FixtureTool : public frame::Tool {
protected:
    /// Configuration setting supplying the default fixture output directory.
    constexpr static auto cOutputDirectoryValue = "output_directory"_el;

public: // implement Tool
    [[nodiscard]] auto optionModule() const -> el::OptionModulePtr override;
    [[nodiscard]] auto configurationRules() const -> el::conf::vr::RulesPtr override;
    void configure(const el::conf::ValuePtr &configuration, const el::OptionValuesPtr &values) override;

protected: // fixture generation
    /// Get the project-relative default output directory.
    [[nodiscard]] virtual auto defaultDirectory() const -> el::String = 0;
    /// Validate every destination before preparing outputs.
    /// @param names Relative fixture paths.
    void validateOutputs(const el::StringList &names) const;
    /// Render a space-separated table with a generated header.
    /// @param schema The column names in output order.
    /// @param rows Structured field lists in row order.
    [[nodiscard]] auto renderRows(const el::String &schema, const el::render::ValueList &rows) const -> el::String;
    /// Publish the prepared files, or fail on differences in check mode.
    void publish() const;
    /// Report an invalid output setting with its original context.
    /// @param description The reason validation failed.
    /// @param cause An optional underlying file-access error.
    [[noreturn]] void throwOutputError(const el::String &description, const std::exception_ptr &cause = {}) const;

protected:
    el::Path _outputDirectory;            ///< Validated output root.
    el::StringMap<el::String> _outputs;   ///< Complete proposed files, keyed by relative path.
    el::render::EnvironmentPtr _renderer; ///< Shared compiled text-template environment.
    bool _checkOnly{};                    ///< Whether publication only compares files.

private:
    el::OptionValuePtr _outputOption; ///< Origin of a command-line override.
    el::conf::ValuePtr _outputConfig; ///< Origin of the configured default.
};

}
