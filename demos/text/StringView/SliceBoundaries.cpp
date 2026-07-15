// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Byte slicing is bounds-safe, but it does not validate UTF-8 boundaries for
/// you.
///
/// A range outside the string becomes an empty view, and an overly long range
/// is clamped to the available text. If a range starts or ends inside a UTF-8
/// sequence, the resulting view contains invalid UTF-8 and later decoding
/// yields replacement characters. Keep byte indexes on character boundaries by
/// using indexes returned by the string API, or by moving them with
/// `advance()` and `retreat()`.
void sliceBoundaries() {
    const auto text = el::StringView{"AåB"_el};

    // Out-of-range slices are safe and simply produce an empty view.
    const auto outside = text.slice(el::ByteRange{el::ByteIndex{99U}, el::ByteLength{5U}});

    // Overly long ranges are clamped to the available storage range.
    const auto clamped = text.slice(el::ByteRange{el::ByteIndex{1U}, el::ByteLength{99U}});

    // Bad code: byte index 2 is in the middle of the UTF-8 sequence for "å".
    const auto broken = text.slice(el::ByteRange{el::ByteIndex{2U}, el::ByteLength{1U}});

    el::io::printLine("Text: "_el, text);
    el::io::printLine("Outside slice is empty: "_el, outside.isEmpty());
    el::io::printLine("Clamped slice: "_el, clamped);
    el::io::printLine("Broken slice: "_el, broken);
    el::io::printLine("Broken slice is valid UTF-8: "_el, broken.isValidUtf8());
}

}
