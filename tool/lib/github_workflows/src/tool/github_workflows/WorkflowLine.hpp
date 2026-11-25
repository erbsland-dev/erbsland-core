// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/String.hpp>

namespace tool::github_workflows {

/// One physical workflow line with its exact original terminator.
struct WorkflowLine {
    el::String text;   ///< Bytes excluding the newline.
    el::String ending; ///< CRLF, LF, CR, or empty for an unterminated final line.
};

}
