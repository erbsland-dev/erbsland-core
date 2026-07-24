// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/text/base_n/BaseNDecoder.hpp>
#include <erbsland/text/base_n/BaseNEncoder.hpp>
#include <erbsland/text/base_n/BaseNFormat.hpp>

namespace demo {

/// Store a binary digest in a text file using an explicit Base-N encoding.
///
/// Base16 is easy to inspect and uses two characters per digest byte. Base64 is more compact when the surrounding
/// format permits it. Decode untrusted text with the expected digest size as a hard output limit.
void storeDigest() {
    auto hasher = el::Hasher{el::HashAlgorithm::Sha3_256};
    hasher.update("Inventaire: orchidée miniature, parcelle Émeraude-4"_el);
    const auto digest = hasher.finalize();

    const auto format = el::text::base_n::BaseNFormat::base16();
    const auto encoded = el::text::base_n::BaseNEncoder{digest, format}.toString();

    auto directoryOptions = el::PathTempDirectoryOptions{};
    directoryOptions.setPrefix("canopée-"_el).setSuffix("-hash-demo"_el);
    const auto directory =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(directoryOptions);
    const auto sidecarPath = directory->path() / "observation.sha3-256"_el;
    sidecarPath.content().writeTextOrThrow(encoded);

    const auto storedText = sidecarPath.content().readTextOrThrow(el::PathReadTextOptions{el::ByteLength{128U}});
    const auto decoded =
        el::text::base_n::BaseNDecoder{storedText, format}.toDataOrThrow(hasher.algorithm().digestSize());

    el::io::printLine("Stored characters: "_el, storedText.characterLength().toSizeT());
    el::io::printLine("Round trip matches: "_el, el::BooleanFormat::yesNo(), decoded == digest);
}

}
