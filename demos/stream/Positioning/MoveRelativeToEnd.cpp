// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Move relative to the start, logical current position, or current end.
/// Relative-to-end movement is useful for fixed trailers without first querying the native file length.
void moveRelativeToEnd() {
    const auto directory = createStreamDemoDirectory("尾部"_el);
    const auto path = directory->path() / "tracks.bin"_el;
    path.content().writeDataOrThrow(el::ByteBlock{std::vector<uint8_t>{1U, 2U, 3U, 4U, 90U, 91U}});
    const auto input = path.content().openByteInputStream();

    input->movePosition(el::StreamPositionOrigin::End, el::ByteOffset{-2});
    const auto trailer = input->readExact(el::ByteLength{2U});
    el::io::printLine(
        "Trailer marker: "_el,
        trailer.data().get(el::ByteIndex{0U}).toUInt8(),
        " / "_el,
        trailer.data().get(el::ByteIndex{1U}).toUInt8());
}

}
