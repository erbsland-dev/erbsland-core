// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Set an absolute encoded-byte position before reading a fixed record.
/// Successful input positioning discards read-ahead and retained aggregate state, so the next read starts cleanly.
void readRecordAtOffset() {
    const auto directory = createStreamDemoDirectory("记录"_el);
    const auto path = directory->path() / "animals.bin"_el;
    path.content().writeDataOrThrow(el::ByteBlock{std::vector<uint8_t>{10U, 11U, 12U, 20U, 21U, 22U}});
    const auto input = path.content().openByteInputStream();

    input->setPosition(el::ByteIndex{3U});
    const auto record = input->readExact(el::ByteLength{3U});
    el::io::printLine("First byte of the second record: "_el, record.data().get(el::ByteIndex{0U}).toUInt8());
}

}
