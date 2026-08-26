// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringDemos.hpp"

namespace demo {

/// Select canonical or compatibility normalization according to the purpose of the text.
void normalizationForms() {
    const auto decomposed = el::String{"Cafe\u0301"_el};
    const auto nfc = decomposed.normalized(el::NormalizationForm::Nfc);
    const auto nfd = nfc.normalized(el::NormalizationForm::Nfd);

    const auto fullWidth = el::String{"ＳＥＮＳＯＲ－７"_el};
    const auto nfkc = fullWidth.normalized(el::NormalizationForm::Nfkc);
    const auto nfkd = fullWidth.normalized(el::NormalizationForm::Nfkd);

    el::io::printLine("Canonical forms:"_el);
    el::io::printLine("  source : "_el, decomposed, " (", decomposed.characterLength(), " code points)"_el);
    el::io::printLine("  NFC ...: "_el, nfc, " (", nfc.characterLength(), " code points)"_el);
    el::io::printLine("  NFD ...: "_el, nfd, " (", nfd.characterLength(), " code points)"_el);
    el::io::printLine("Compatibility forms:"_el);
    el::io::printLine("  source : "_el, fullWidth);
    el::io::printLine("  NFKC ..: "_el, nfkc);
    el::io::printLine("  NFKD ..: "_el, nfkd);
}

}
