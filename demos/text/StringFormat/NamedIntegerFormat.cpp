// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringFormatDemos.hpp"

namespace demo {

/// Named number specifications can lock a field to integers and describe its base and layout explicitly.
void namedIntegerFormat() {
    const auto packetId =
        el::StringFormat{"{:number:base=hexadecimal,alternate,letter-case=uppercase,width=10,zero-fill}"_el};
    const auto signedCount = el::StringFormat{"{:number:base=decimal,sign=always,width=7,alignment=right}"_el};

    el::io::printLine("Packet id ...: "_el, packetId.build(42));
    el::io::printLine("Difference ..: "_el, signedCount.build(17));
}

}
