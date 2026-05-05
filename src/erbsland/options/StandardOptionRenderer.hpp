// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionErrorContext_fwd.hpp"
#include "OptionHelp.hpp"
#include "OptionModule_fwd.hpp"
#include "OptionRenderer.hpp"
#include "Options_fwd.hpp"
#include "OptionSet_fwd.hpp"
#include "StandardOptionRenderer_fwd.hpp"

#include "../stream/TextOutputStream.hpp"
#include "../text/String.hpp"

#include <vector>

namespace erbsland::options {

/// Plain text renderer for option help, version text, and errors.
/// @tested{StandardOptionRendererTest}
class StandardOptionRenderer final : public OptionRenderer {
public:
    /// Create a renderer that writes to the process standard streams.
    StandardOptionRenderer();
    /// Create a renderer with explicit output and error streams.
    StandardOptionRenderer(stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error);

    // defaults
    ~StandardOptionRenderer() override = default;
    StandardOptionRenderer(const StandardOptionRenderer &) = default;
    auto operator=(const StandardOptionRenderer &) -> StandardOptionRenderer & = default;
    StandardOptionRenderer(StandardOptionRenderer &&) = default;
    auto operator=(StandardOptionRenderer &&) -> StandardOptionRenderer & = default;

public:
    /// Create a shared standard option renderer.
    [[nodiscard]] static auto create() -> StandardOptionRendererPtr;
    /// Create a shared standard option renderer with explicit output and error streams.
    [[nodiscard]] static auto create(stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error)
        -> StandardOptionRendererPtr;

public: // implement OptionRenderer
    void displayHelp(const OptionsPtr &options, text::StringView moduleName) override;
    void displayVersion(const OptionsPtr &options, text::StringView moduleName) override;
    void displayError(const OptionsPtr &options, const OptionErrorContext &errorContext) override;

private:
    struct TextRow {
        text::String title;
        text::String description;
        std::vector<TextRow> details;
    };

private:
    [[nodiscard]] auto findModule(const OptionsPtr &options, const text::StringView &moduleName) const
        -> OptionModulePtr;
    [[nodiscard]] auto visibleOptionSets(const OptionsPtr &options, const OptionModulePtr &module) const
        -> std::vector<OptionSetPtr>;
    [[nodiscard]] auto optionRows(const std::vector<OptionSetPtr> &optionSets) const -> std::vector<TextRow>;
    [[nodiscard]] auto moduleRows(const OptionsPtr &options) const -> std::vector<TextRow>;
    [[nodiscard]] auto choiceRows(const OptionPtr &option) const -> std::vector<TextRow>;
    [[nodiscard]] auto optionTitle(const OptionPtr &option) const -> text::String;
    [[nodiscard]] auto optionDescription(const OptionPtr &option) const -> text::String;
    [[nodiscard]] auto visibleHelp(const OptionHelp &help) const noexcept -> bool;
    [[nodiscard]] auto visibleOptionSet(const OptionSetPtr &optionSet) const noexcept -> bool;
    [[nodiscard]] auto displayName(const OptionsPtr &options) const -> text::StringView;
    [[nodiscard]] auto titleText(const OptionsPtr &options, const OptionModulePtr &module) const -> text::StringView;
    [[nodiscard]] auto selectedHelp(const OptionsPtr &options, const OptionModulePtr &module) const
        -> const OptionHelp *;
    void writeUsage(const OptionsPtr &options, const OptionModulePtr &module);
    void writeHelpText(const OptionHelp &help);
    void writeSection(text::StringView title, const std::vector<TextRow> &rows);
    void writeRows(const std::vector<TextRow> &rows, text::StringView indent);
    void writeRow(const TextRow &row, text::StringView indent);
    void writeDetailRows(const std::vector<TextRow> &rows, text::StringView indent);

private:
    stream::TextOutputStreamPtr _output; ///< The stream used for help and version output.
    stream::TextOutputStreamPtr _error;  ///< The stream used for error output.
};

}
