// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FormatPart.hpp"

#include "../../mem/SharedData.hpp"
#include "../../mem/SharedDataPointer.hpp"
#include "../../unit/ArgumentUnit.hpp"

#include <vector>

namespace erbsland::text::impl {

/// The immutable shared data of a compiled UTF-8 format.
/// @tested{U8FormatTest}
class FormatData final : public mem::SharedData {
public:
    std::vector<FormatPart> parts;                                  ///< The compiled format parts.
    std::vector<bool> usedArguments;                                ///< The argument indexes used by fields.
    unit::ArgumentCount fieldCount{unit::ArgumentCount::zero()};    ///< The number of argument fields.
    unit::ArgumentCount argumentCount{unit::ArgumentCount::zero()}; ///< The required number of arguments.
};

/// Shared pointer to compiled UTF-8 format data.
using FormatDataPtr = mem::SharedDataPointer<FormatData>;

}
