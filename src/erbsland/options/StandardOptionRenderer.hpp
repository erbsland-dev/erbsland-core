// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Option_fwd.hpp"
#include "OptionDisplayText.hpp"
#include "OptionErrorContext_fwd.hpp"
#include "OptionHelp.hpp"
#include "OptionModule_fwd.hpp"
#include "OptionRendererBase.hpp"
#include "Options_fwd.hpp"
#include "OptionSet_fwd.hpp"
#include "StandardOptionRenderer_fwd.hpp"

#include "impl/OptionDisplayModel.hpp"

#include "../stream/TextOutputStream.hpp"

#include <vector>

namespace erbsland::options {

/// Plain text renderer for option help, version text, and errors.
/// @tested{StandardOptionRendererTest}
class StandardOptionRenderer final : public OptionRendererBase {
public:
    /// Create a renderer that writes to the process standard streams.
    StandardOptionRenderer();
    /// Create a renderer with explicit output and error streams.
    StandardOptionRenderer(stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error);
    /// Create a renderer with explicit streams and display text.
    StandardOptionRenderer(
        stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error, OptionDisplayText displayText);

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
    /// Create a shared standard option renderer with explicit streams and display text.
    [[nodiscard]] static auto create(
        stream::TextOutputStreamPtr output, stream::TextOutputStreamPtr error, OptionDisplayText displayText)
        -> StandardOptionRendererPtr;

public: // implement OptionRenderer
    void displayHelp(const OptionsPtr &options, text::StringView moduleName) override;
    void displayVersion(const OptionsPtr &options, text::StringView moduleName) override;
    void displayError(const OptionsPtr &options, const OptionErrorContext &errorContext) override;

private:
    void writeUsage(const impl::OptionDisplayModel &model);
    void writeHelpText(const OptionHelp &help);
    void writeSection(text::StringView title, const std::vector<impl::OptionDisplayRow> &rows);
    void writeRows(const std::vector<impl::OptionDisplayRow> &rows, text::StringView indent);
    void writeRow(const impl::OptionDisplayRow &row, text::StringView indent);
    void writeDetailRows(const std::vector<impl::OptionDisplayRow> &rows, text::StringView indent);

private:
    stream::TextOutputStreamPtr _output; ///< The stream used for help and version output.
    stream::TextOutputStreamPtr _error;  ///< The stream used for error output.
};

}
