// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// `normalize()` canonicalizes an editor in place. This decomposed Japanese
/// element name uses a combining voiced mark, which NFC composes with the
/// preceding katakana character.
void normalizeText() {
    auto elementName = el::StringEditor{"カ\u3099リウム"_el};

    el::io::printLine("Before NFC: "_el, elementName);
    elementName.normalize(el::NormalizationForm::Nfc);
    el::io::printLine("After NFC : "_el, elementName);
}

}
