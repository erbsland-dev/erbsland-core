// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Terminate a dynamic text field with an encoded character.
///
/// Clearing the count format and setting an end mark creates a delimited field.
/// The configured maximum length bounds the search when reading untrusted data.
void useEndMark() {
    auto options = el::ByteTextOptions{};
    options.clearCountFormat().setEndMark(U'|').setLength(el::ByteLength{16U});
    auto writer = el::ByteWriter{};

    // Encode two moon phases whose boundaries are marked by a vertical bar.
    writer.writeTextOrThrow("pleine"_el, options).writeTextOrThrow("nouvelle"_el, options);
    const auto bytes = writer.toByteBlock();
    auto reader = el::ByteReader{bytes};

    el::io::printLine("First phase       : "_el, reader.readTextOrThrow(options));
    el::io::printLine("Second phase      : "_el, reader.readTextOrThrow(options));
    el::io::printLine("Encoded fields    : "_el, el::ByteFormat::separated(), bytes);
    el::io::printLine("Has end mark      : "_el, el::BooleanFormat::yesNo(), options.endMark().has_value());
}

}
