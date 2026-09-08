// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Navigate a byte stream without reading past its boundary.
///
/// `position()` and `length()` describe the cursor and input size. `canRead()`
/// validates a prospective read, while `advance()` and `setPosition()` move the
/// cursor and clamp it to the valid range.
void navigateReader() {
    const auto record = el::ByteBlock{el::Byte{0x4eU}, el::Byte{0x00U}, el::Byte{0x0cU}, el::Byte{94U}};
    auto reader = el::ByteReader{record};

    // Skip the marker after verifying that the complete header is present.
    const auto hasHeader = reader.canRead(el::ByteLength{3U});
    reader.advance(el::ByteLength{1U});
    const auto fieldPosition = reader.position();

    // A position beyond the input is safely clamped to its end.
    reader.setPosition(el::ByteIndex{100U});
    el::io::printLine("Header available  : "_el, el::BooleanFormat::yesNo(), hasHeader);
    el::io::printLine("Field position    : "_el, fieldPosition.toSizeT());
    el::io::printLine("Input length      : "_el, reader.length().toSizeT());
    el::io::printLine("At end            : "_el, el::BooleanFormat::yesNo(), reader.isAtEnd());
}

}
