// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Throw `ApplicationError` from code executed by `Application::run()` to request consistent error reporting.
/// Its context provides a title, description, source information, location, and the process exit code.
void applicationReporting() {
    auto context = el::ApplicationErrorContext{
        "The score could not be read."_el,
        "The main melody contains an unrecognized symbol."_el,
        el::ExitCode::failure(),
    };
    context.setSourceName("春の合奏"_el)
        .setSourcePath("scores/春の合奏.music"_el)
        .setCodeLocation(el::CodeLocation{.line = el::LineIndex{11}, .column = el::ColumnIndex{8}});
    throw el::ApplicationError{std::move(context), {}};
}

}
