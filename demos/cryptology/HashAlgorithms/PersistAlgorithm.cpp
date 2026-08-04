// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/conf/Parser.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/cryptology/HashSelector.hpp>

namespace demo {

/// Persist the stable algorithm identifier beside every digest that must be interpreted later.
///
/// Parse identifiers with `HashAlgorithm::fromString()`, apply current safety policy, and verify that the stored digest
/// has the size required by the selected algorithm before using the record.
void persistAlgorithm() {
    const auto algorithm = el::HashAlgorithm{el::HashAlgorithm::Sha3_256};
    auto hasher = el::Hasher{algorithm};
    hasher.update("Relevé: paresseux à trois doigts, hauteur 28 m"_el);
    const auto digest = hasher.finalize();

    const auto configuration = el::StringFormat{"[canopy_record]\n"
                                                "algorithm: \"{}\"\n"
                                                "digest: <{}>\n"_el}
                                   .build(algorithm.toString(), digest);
    const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

    const auto algorithmText = document->getTextOrThrow(el::String{"canopy_record.algorithm"_el});
    const auto storedAlgorithm = el::HashAlgorithm::fromString(algorithmText);
    if (!storedAlgorithm.has_value() || !el::HashSelector{}.isSafe(storedAlgorithm.value())) {
        throw el::RuntimeError{"The stored hash algorithm is unknown or no longer acceptable."_el};
    }

    const auto storedDigest = document->getBytesOrThrow(el::String{"canopy_record.digest"_el});
    if (storedDigest.length() != storedAlgorithm->digestSize()) {
        throw el::RuntimeError{"The digest size does not match the stored hash algorithm."_el};
    }

    el::io::printLine("Algorithm accepted: "_el, storedAlgorithm->toString());
    el::io::printLine("Digest size valid: "_el, el::BooleanFormat::yesNo(), storedDigest == digest);
}

}
