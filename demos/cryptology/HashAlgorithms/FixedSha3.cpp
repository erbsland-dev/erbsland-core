// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cryptology/Hasher.hpp>

namespace demo {

/// `Hasher` calculates a fixed-output cryptographic digest incrementally.
///
/// Select an explicit algorithm when a file format or protocol defines it, add the exact bytes in one or more calls,
/// and call `finalize()` after the complete message has been processed. Text updates hash the exact UTF-8 bytes stored
/// in the string.
void fixedSha3() {
    auto hasher = el::Hasher{el::HashAlgorithm::Sha3_256};

    // Feed one logical observation to the hasher in two pieces.
    hasher.update("Site: canopée nord; espèce: toucan à bec rouge; "_el);
    hasher.update("humidité: 87 %"_el);
    const auto digest = hasher.finalize();

    el::io::printLine("Algorithm: "_el, hasher.algorithm().toString());
    el::io::printLine("Digest: "_el, digest);
}

}
