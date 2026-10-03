// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace erbsland::i18n::compiler {

/// Rendered catalog files, ready for writing or comparison.
/// @tested{DisplayTextCompilerTest}
struct DisplayTextOutput {
    text::String header; ///< Complete header text.
    text::String source; ///< Complete implementation text.
};

}
