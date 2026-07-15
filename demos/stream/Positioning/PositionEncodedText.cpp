// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

namespace demo {

/// Text positions count encoded bytes and must land on a code-point boundary.
/// Explicit UTF-16 or UTF-32 byte order is best for immediate random access because no BOM must first resolve it.
void positionEncodedText() {
    const auto directory = createStreamDemoDirectory("文本"_el);
    const auto path = directory->path() / "species.txt"_el;
    auto writeOptions = el::PathWriteTextOptions{el::StringEncoding::Utf16LittleEndian};
    writeOptions.setBomMode(el::StringBomMode::Reject);
    const auto output = path.content().openTextOutputStream(writeOptions);
    output->write("鹿狐熊"_el);
    output->close();

    auto readOptions = el::PathReadTextOptions{el::StringEncoding::Utf16LittleEndian};
    readOptions.setBomMode(el::StringBomMode::Reject);
    const auto input = path.content().openTextInputStream(readOptions);
    input->setPosition(el::ByteIndex{2U});
    el::io::printLine("Second character: "_el, input->readChar().data());
}

}
