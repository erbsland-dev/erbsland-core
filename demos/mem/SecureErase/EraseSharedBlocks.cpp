// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Securely erase copy-on-write byte blocks without overlooking aliases.
///
/// Marking the allocation as sensitive before it receives secret bytes ensures
/// final cleanup. An explicit erase zeros the invoking value immediately, but a
/// shared alias remains readable until it is erased or released.
void eraseSharedBlocks() {
    auto workingKey = el::ByteBlockEditor{el::ByteLength{6U}};
    workingKey.markAsSensitive();

    // Mark allocated storage before writing the sensitive value.
    workingKey.set(el::ByteIndex{0U}, el::Byte{0x61U});
    workingKey.set(el::ByteIndex{1U}, el::Byte{0x73U});
    workingKey.set(el::ByteIndex{2U}, el::Byte{0x74U});
    workingKey.set(el::ByteIndex{3U}, el::Byte{0x72U});
    workingKey.set(el::ByteIndex{4U}, el::Byte{0x6fU});
    workingKey.set(el::ByteIndex{5U}, el::Byte{0x69U});

    // This read-only block shares the marked allocation with the editor.
    auto retainedKey = el::ByteBlock{workingKey};
    workingKey.secureErase();
    const auto workingCopyIsZero = workingKey.isEqualConstTime(el::ByteBlock{workingKey.length()});
    const auto retainedCopyStillExists = retainedKey.get(el::ByteIndex::zero()) != el::Byte{};

    // Erase the remaining owner when it is no longer needed.
    retainedKey.secureErase();
    const auto retainedCopyIsZero = retainedKey.isEqualConstTime(el::ByteBlock{retainedKey.length()});

    el::io::printLine("Nøgle              : Asteroide-session"_el);
    el::io::printLine("Editor erased      : "_el, el::BooleanFormat::yesNo(), workingCopyIsZero);
    el::io::printLine("Alias retained data: "_el, el::BooleanFormat::yesNo(), retainedCopyStillExists);
    el::io::printLine("Alias erased       : "_el, el::BooleanFormat::yesNo(), retainedCopyIsZero);
}

}
