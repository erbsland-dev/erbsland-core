// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::options {

/// The status of option processing.
enum class OptionResultStatus {
    Success,        ///< Parsing succeeded.
    DisplayVersion, ///< The version should be displayed.
    DisplayHelp,    ///< Help should be displayed.
    Error,          ///< Parsing failed.
};

}
