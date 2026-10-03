// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteFormat.hpp>
#include <erbsland/ByteReader.hpp>
#include <erbsland/ByteTextOptions.hpp>
#include <erbsland/ByteWriter.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

/// Encode framed text into a byte stream.
///
/// The default options write a length-prefixed UTF-8 field. `writeText()` may
/// truncate at a character boundary when a configured limit requires it, while
/// `writeTextOrThrow()` requires an exact representation.
void writeText() {
    auto options = el::ByteTextOptions::compact();
    options.setLength(el::ByteCount{16U});
    auto writer = el::ByteWriter{};

    // Encode two compact UTF-8 fields using identical framing.
    writer.writeTextOrThrow("bosuil"_el, options).writeTextOrThrow("maanlicht"_el, options);
    const auto record = writer.toByteBlock();

    // Read the fields back with the same options.
    auto reader = el::ByteReader{record};
    el::io::printLine("Soort             : "_el, reader.readTextOrThrow(options));
    el::io::printLine("Omgeving          : "_el, reader.readTextOrThrow(options));
    el::io::printLine("Encoded fields    : "_el, el::ByteFormat::separated(), record);
}

}
