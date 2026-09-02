// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

namespace erbsland::options {

/// The concrete storage type of an option value.
enum class OptionValueType {
    Flag,          ///< A valueless flag occurrence.
    Boolean,       ///< A single boolean value.
    BooleanList,   ///< A list of boolean values.
    Integer,       ///< A single integer value.
    IntegerList,   ///< A list of integer values.
    Text,          ///< A single text value.
    TextList,      ///< A list of text values.
    SensitiveText, ///< A single protected sensitive text value.
};

}
