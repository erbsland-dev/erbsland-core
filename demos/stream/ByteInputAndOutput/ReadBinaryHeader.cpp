// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <exception>

namespace demo {

void inspectBinaryHeader(el::ByteInputStream &input);

void readBinaryHeader() {
    auto input = ScriptedByteInputStream{{0x47U, 0x45U, 0x4fU, 0x31U, 0x03U, 0x00U}, 2U};
    inspectBinaryHeader(input);
}

/// Handle every result of an exact binary-header read.
/// `Data`, `Finished`, and `Timeout` are expected result states, while a `StreamError` reports a failed source.
void inspectBinaryHeader(el::ByteInputStream &input) {
    try {
        const auto result = input.readExact(el::ByteLength{4U});

        if (result.isTimeout()) {
            el::io::printLine("The geometry-file header is temporarily unavailable."_el);
            return;
        }
        if (result.isFinished()) {
            el::io::printLine("The geometry file does not contain a complete header."_el);
            return;
        }

        const auto &header = result.data();
        el::io::printLine(
            "Geometry-file marker: "_el, el::String{el::String::fromByteBlock(header, el::ByteFormat::separated())});
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The geometry-file header could not be read."_el, std::current_exception()};
    }
}

}
