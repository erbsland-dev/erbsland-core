// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Select a Unicode encoding and its byte order independently from the stream.
///
/// The reader or writer endianness controls an integer count prefix. The byte
/// order in an explicit UTF-16 or UTF-32 encoding controls the text code units.
void selectEncoding() {
    auto options = el::ByteTextOptions{};
    options.setEncoding(el::StringEncoding::Utf16BigEndian).setCountFormat(el::ByteIntegerFormat::UnsignedFixed16Bit);
    auto writer = el::ByteWriter{};
    writer.setEndianness(el::Endianness::Little);

    // Write a little-endian count followed by big-endian UTF-16 code units.
    writer.writeTextOrThrow("éclipse"_el, options);
    const auto bytes = writer.toByteBlock();
    auto reader = el::ByteReader{bytes};
    reader.setEndianness(el::Endianness::Little);

    el::io::printLine("Decoded phase     : "_el, reader.readTextOrThrow(options));
    el::io::printLine("Encoded bytes     : "_el, el::ByteFormat::separated(), bytes);
    el::io::printLine(
        "UTF-16 big endian : "_el,
        el::BooleanFormat::yesNo(),
        options.encoding() == el::StringEncoding::Utf16BigEndian);
}

}
