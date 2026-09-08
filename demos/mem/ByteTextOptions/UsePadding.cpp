// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Fill the unused bytes of a fixed text field with a chosen padding byte.
///
/// A count or end mark identifies the payload inside the field. The reader then
/// consumes the complete field, including padding, before reading the next value.
void usePadding() {
    auto options = el::ByteTextOptions{el::ByteTextFormat::PaddedField};
    options.clearCountFormat().setEndMark(U'|').setLength(el::ByteLength{12U}).setPadding(el::Byte{0x2eU});
    auto writer = el::ByteWriter{};

    // Store one phase in a twelve-byte, dot-padded field.
    writer.writeTextOrThrow("lune"_el, options);
    const auto bytes = writer.toByteBlock();
    auto reader = el::ByteReader{bytes};

    el::io::printLine("Decoded phase     : "_el, reader.readTextOrThrow(options));
    el::io::printLine("Padded field      : "_el, el::ByteFormat::separated(), bytes);
    el::io::printLine("Padding byte      : "_el, options.padding().toUInt32());
    el::io::printLine("Reader at end     : "_el, el::BooleanFormat::yesNo(), reader.isAtEnd());
}

}
