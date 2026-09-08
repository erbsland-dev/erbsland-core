// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Decode framed text from a byte stream.
///
/// `ByteTextOptions` defines both the character encoding and the byte-level
/// framing. Optional reading leaves the position unchanged if a complete valid
/// field is unavailable.
void readText() {
    const auto record = el::ByteBlock{
        el::Byte{0x06U},
        el::Byte{0x00U},
        el::Byte{0x00U},
        el::Byte{0x00U},
        el::Byte{'b'},
        el::Byte{'o'},
        el::Byte{'s'},
        el::Byte{'u'},
        el::Byte{'i'},
        el::Byte{'l'}};
    auto reader = el::ByteReader{record};

    // Decode the default length-prefixed UTF-8 field.
    const auto species = reader.readTextOrThrow();
    const auto missingField = reader.readText();
    el::io::printLine("Soort              : "_el, species);
    el::io::printLine("Complete next field: "_el, el::BooleanFormat::yesNo(), missingField.has_value());
}

}
