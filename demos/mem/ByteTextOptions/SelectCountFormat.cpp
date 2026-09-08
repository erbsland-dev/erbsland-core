// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Select the integer representation used for a text code-unit count.
///
/// A fixed-width count is simple to inspect and overwrite. A variable-length
/// count makes short text fields compact while still supporting large values.
void selectCountFormat() {
    const auto fixedOptions = el::ByteTextOptions{};
    auto compactOptions = fixedOptions;
    compactOptions.setCountFormat(el::ByteIntegerFormat::UnsignedVariableLength);

    // Encode the same phase with a 32-bit and a variable-length count.
    auto fixedWriter = el::ByteWriter{};
    fixedWriter.writeTextOrThrow("lune"_el, fixedOptions);
    auto compactWriter = el::ByteWriter{};
    compactWriter.writeTextOrThrow("lune"_el, compactOptions);

    el::io::printLine("Fixed count       : "_el, el::ByteFormat::separated(), fixedWriter.toByteBlock());
    el::io::printLine("Compact count     : "_el, el::ByteFormat::separated(), compactWriter.toByteBlock());
    el::io::printLine("Count configured  : "_el, el::BooleanFormat::yesNo(), compactOptions.countFormat().has_value());
}

}
