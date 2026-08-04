// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cryptology/HashSelector.hpp>

namespace demo {

/// `HashSelector` selects a supported algorithm from application requirements and current policy.
///
/// Use `recommended()` when your application controls the format and can follow current library policy. Persist the
/// returned algorithm identifier with the digest because recommendations and metadata can change in later releases.
void selectAlgorithm() {
    const auto requirements = el::HashRequirements{
        .requiredStatus = el::CryptographicStatus::Acceptable,
        .minimumSecurity = el::CryptographicSecurity::High,
        .minimumThroughput = el::HashThroughput::Medium,
    };
    const auto algorithm = el::HashSelector{requirements}.recommended();

    if (!algorithm.has_value()) {
        el::io::printLine("No supported hash algorithm satisfies the requirements."_el);
        return;
    }

    el::io::printLine("Selected algorithm: "_el, algorithm->toString());
    el::io::printLine("Digest bytes: "_el, algorithm->digestSize().toSizeT());
}

}
