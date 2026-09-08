// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Select dynamic or fixed-field framing for encoded text.
///
/// `ByteTextOptions::setFormat()` changes how the reader finds the end of a
/// field. Dynamic fields carry their own boundary, while padded fields occupy
/// exactly the configured byte length.
void selectFormat() {
    auto dynamicOptions = el::ByteTextOptions::compact();
    auto paddedOptions = dynamicOptions;
    paddedOptions.setFormat(el::ByteTextFormat::PaddedField).setLength(el::ByteLength{12U}).setPadding(el::Byte{0x20U});

    // Encode the same moon phase using dynamic and fixed-field framing.
    auto dynamicWriter = el::ByteWriter{};
    dynamicWriter.writeTextOrThrow("croissant"_el, dynamicOptions);
    auto paddedWriter = el::ByteWriter{};
    paddedWriter.writeTextOrThrow("croissant"_el, paddedOptions);

    el::io::printLine("Phase             : croissant"_el);
    el::io::printLine("Dynamic bytes     : "_el, el::ByteFormat::separated(), dynamicWriter.toByteBlock());
    el::io::printLine("Padded bytes      : "_el, el::ByteFormat::separated(), paddedWriter.toByteBlock());
    el::io::printLine(
        "Fixed field       : "_el,
        el::BooleanFormat::yesNo(),
        paddedOptions.format() == el::ByteTextFormat::PaddedField);
}

}
