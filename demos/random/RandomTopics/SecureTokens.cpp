// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/CharSet.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

/// `SecureRandom` is the generator for secrets and security boundaries.
///
/// It draws bytes from the operating system entropy source and throws
/// `RandomFailure` if secure random data is unavailable. Generate protocol-visible
/// tokens with an explicit character set, and use protected values for secrets
/// that do not have to be exposed.
void secureTokens() {
    const auto tokenAlphabet = el::CharSet::fromPattern("A-Za-z0-9"_el);

    // Field notebooks use a protected access code and a binary sealing key.
    auto &secureRandom = el::application().secureRandom();
    const auto notebookCode = secureRandom.buildString(el::CpCount{32U}, tokenAlphabet);
    const auto sealingKey = secureRandom.buildByteBuffer(el::ByteCount{32U});

    el::io::printLine("Feltbog token length : "_el, notebookCode.characterLength().toSizeT());
    el::io::printLine("Binary key bytes     : "_el, sealingKey.length().toSizeT());
}

}
