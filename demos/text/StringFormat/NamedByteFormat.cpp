// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringFormatDemos.hpp"

#include <cstdint>
#include <vector>

namespace demo {

/// Named byte specifications make separators and bounded diagnostic output explicit.
void namedByteFormat() {
    const auto packet = el::mem::ByteBlock::fromVector(
        std::vector<uint8_t>{0x10U, 0x21U, 0x32U, 0x43U, 0x54U, 0x65U, 0x76U, 0x87U, 0x98U});
    const auto full = el::StringFormat{"{:bytes:separator}"_el};
    const auto diagnostic = el::StringFormat{"{:bytes:separator,maximum=6,truncate=middle}"_el};

    el::io::printLine("Complete ......: "_el, full.build(packet));
    el::io::printLine("Diagnostic ....: "_el, diagnostic.build(packet));
}

}
