// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "StringDemos.hpp"

namespace demo {

/// Read UTF-8 sequentially in either direction while keeping one native byte index.
void sequentialCharacterReading() {
    const auto label = el::String{"veld🌱"_el};

    auto forwardIndex = el::ByteIndex::zero();
    auto forward = el::StringEditor{};
    while (true) {
        const auto character = label.readCharAndAdvance(forwardIndex);
        if (character.isEndOfData()) {
            break;
        }
        forward.append(character);
    }

    auto reverseIndex = label.indexAt(el::StringSide::Back);
    auto reverse = el::StringEditor{};
    while (true) {
        const auto character = label.readCharAndRetreat(reverseIndex);
        if (character.isEndOfData()) {
            break;
        }
        reverse.append(character);
    }

    el::io::printLine("Text ...........: "_el, label);
    el::io::printLine("Forward ........: "_el, forward);
    el::io::printLine("Reverse ........: "_el, reverse);
    el::io::printLine("Forward index ..: "_el, forwardIndex);
    el::io::printLine("Reverse index ..: "_el, reverseIndex);
}

}
