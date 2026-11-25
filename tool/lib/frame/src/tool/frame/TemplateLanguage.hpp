// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace tool::frame {

/// The language for render environments
enum class TemplateLanguage : uint8_t {
    Cpp,  ///< An environment for C++
    Text, ///< Plain text with the standard template delimiters.
};

}
