// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "IntegerBase.hpp"
#include "ReadNumberStatus.hpp"

#include "../unit/CpIndex.hpp"
#include "../unit/CpLength.hpp"

#include <cstdint>

namespace erbsland::text {

/// Result for reading an integer from a string reader.
/// @tested{StringCharReaderTest}
struct ReadIntegerResult final {
    std::uint64_t value{};                              ///< The parsed unsigned value.
    unit::CpLength digitCount{unit::CpLength::zero()};  ///< The number of consumed digits.
    bool isNegative{false};                             ///< Whether a minus sign was consumed.
    IntegerBase base{IntegerBase::Decimal};             ///< The resolved integer base.
    unit::CpIndex position{unit::CpIndex::noIndex()};   ///< The start or error position.
    ReadNumberStatus status{ReadNumberStatus::Success}; ///< The result status.
};

}
