// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::options {

/// The structured reason for an option error.
enum class OptionErrorReason : uint8_t {
    None,                ///< No reason given.
    SyntaxError,         ///< The command line syntax is invalid.
    UnknownName,         ///< An option name is not known.
    UnexpectedValueType, ///< A parsed value does not match the expected option type.
    ValidationError,     ///< A callback or validator rejected the parsed options.
    NotImplemented,      ///< The requested operation is not implemented yet.
};

}
