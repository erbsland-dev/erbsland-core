// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodecOutput.hpp"

#include "../../mem/BitWriter.hpp"

namespace erbsland::compression::impl::codecBitOutput {

/// Drain complete bytes while retaining partial bits in their wire bit order.
/// @tested{CompressionStreamingTest}
inline void drain(mem::BitWriter &writer, const CodecOutput::Write &output, bool final = false) {
    const auto bits = writer.bitCount();
    const auto order = writer.bitOrder();
    auto data = writer.takeByteBlockEditor();
    const auto whole = bits / 8U;
    const auto remainder = bits % 8U;
    output(data.span().first(final ? (bits + 7U) / 8U : whole));
    if (!final && remainder) {
        auto value = data.span()[whole].toUInt64();
        if (order == mem::BitOrder::MostSignificantFirst) {
            value >>= 8U - remainder;
        }
        writer.writeBits(value, remainder);
    }
}

}
