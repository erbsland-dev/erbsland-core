// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

/// `SecureRandom` is the generator for secrets and security boundaries.
///
/// It draws bytes from the operating system entropy source and throws
/// `RandomError` if secure random data is unavailable. Generate protocol-visible
/// tokens with an explicit character set, and prefer byte blocks for binary
/// keys, salts, and nonces.
void secureTokens() {
    const auto tokenAlphabet = el::CharSet::fromPattern("A-Za-z0-9"_el);
    const auto yesNo = el::BooleanFormat::yesNo();

    try {
        // Field notebooks use a visible code and a binary sealing key.
        auto &secureRandom = el::application().secureRandom();
        const auto notebookCode = secureRandom.buildString(el::CpLength{32U}, tokenAlphabet);
        const auto sealingKey = secureRandom.buildByteBlock(el::ByteLength{32U});

        el::io::printLine("Feltbog token length : "_el, notebookCode.characterLength().toSizeT());
        el::io::printLine("Allowed alphabet     : "_el, yesNo, notebookCode.containsOnly(tokenAlphabet));
        el::io::printLine("Binary key bytes     : "_el, sealingKey.length().toSizeT());
    } catch (const el::RandomError &error) {
        el::io::printLine("Secure randomness is unavailable: "_el, error.reason());
    }
}
