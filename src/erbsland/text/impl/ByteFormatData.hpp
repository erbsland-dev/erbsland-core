// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteFormatData_fwd.hpp"

#include "../ByteFormatFlag.hpp"
#include "../LetterCase.hpp"
#include "../String.hpp"
#include "../TruncateMode.hpp"

#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::text::impl {

/// Private data stored by a byte format value.
/// @tested{ByteFormatTest}
struct ByteFormatData {
    /// Create byte format data with the standard separators.
    ByteFormatData();

    ByteFormatFlags flags;                                  ///< The active format flags.
    LetterCase letterCase{LetterCase::Lowercase};           ///< The case for ASCII letters.
    unit::ByteLength bytesPerLine{unit::ByteLength{32U}};   ///< The number of bytes per line.
    unit::ByteLength byteGroupSize{unit::ByteLength{4U}};   ///< The number of bytes in a group.
    unit::ItemCount lineGroupSize{unit::ItemCount{8U}};     ///< The number of lines in a group.
    String byteSeparator;                                   ///< The separator between bytes or byte groups.
    String offsetSeparator;                                 ///< The separator between the offset and bytes.
    String linePrefix;                                      ///< The prefix inserted before each byte-data line.
    String lineSuffix;                                      ///< The suffix inserted after each byte-data line.
    unit::ByteIndex startOffset{unit::ByteIndex::zero()};   ///< The start offset for the dump.
    unit::ByteLength maximum{unit::ByteLength::infinite()}; ///< The maximum byte-like output item count.
    TruncateMode truncateMode{TruncateMode::End};           ///< The truncation mode.
    String ellipsis;                                        ///< The text inserted for omitted bytes.
};

}
