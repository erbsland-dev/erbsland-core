// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `diagnostic()` returns the structured description for one exception.
/// Its source accessors let handlers inspect optional metadata without knowing the concrete exception type.
void inspectDiagnostic() {
    auto context = el::ApplicationErrorContext{"The score could not be read."_el};
    context.setSourceName("夜の演奏"_el)
        .setSourcePath("scores/夜の演奏.music"_el)
        .setCodeLocation(el::CodeLocation{.line = el::LineIndex{3}, .column = el::ColumnIndex{5}});
    const auto diagnostic = el::ApplicationError{std::move(context), {}}.diagnostic();
    const auto location = diagnostic->location();

    el::io::printLine("Source: "_el, diagnostic->sourceName());
    el::io::printLine("Path: "_el, diagnostic->sourcePath());
    el::io::printLine("Location: "_el, location.line.toSizeT() + 1, ":"_el, location.column.toSizeT() + 1);
}

}
