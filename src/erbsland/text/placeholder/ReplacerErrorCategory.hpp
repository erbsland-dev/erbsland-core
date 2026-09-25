// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::text::placeholder {

/// The kind of placeholder processing failure.
enum class ReplacerErrorCategory : uint8_t {
    Syntax,        ///< Invalid placeholder or parameter syntax.
    UnexpectedEnd, ///< A placeholder ended without its closing frame.
    LimitExceeded, ///< A configured or expression limit was exceeded.
    Unsupported,   ///< A source or filter is not registered.
    ValueNotFound, ///< A requested value does not exist.
    NameConflict,  ///< Two normalized names refer to different values.
    Validation,    ///< A value failed a required condition.
    Access,        ///< A provider denied access to a value.
};

}
