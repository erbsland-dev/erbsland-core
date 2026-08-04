// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/cryptology/HashSelector.hpp>

#include <algorithm>
#include <array>
#include <span>

namespace demo {

auto hashBoundedStream(el::ByteInputStream &input, el::HashAlgorithm algorithm, std::size_t maximumBytes)
    -> el::ByteBlock;

/// Hash a file through a bounded stream and current algorithm policy.
///
/// Open the file once, process its bytes incrementally, and keep both the memory use and total accepted input bounded.
/// Check `HashSelector::isSafe()` before processing data that names its own algorithm.
void hashBoundedInput() {
    auto directoryOptions = el::PathTempDirectoryOptions{};
    directoryOptions.setPrefix("forêt-"_el).setSuffix("-hash-demo"_el);
    const auto directory =
        el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(directoryOptions);
    const auto observationPath = directory->path() / "inventaire.txt"_el;
    observationPath.content().writeTextOrThrow(
        "Parcelle: Ambre-7\nStrate: canopée\nObservation: deux aras chloroptères\n"_el);

    const auto input = observationPath.content().openByteInputStream();
    const auto digest = hashBoundedStream(*input, el::HashAlgorithm::Sha3_256, 1024U);
    input->close();

    el::io::printLine("Digest bytes: "_el, digest.length().toSizeT());
    el::io::printLine(
        "MD5 rejected: "_el, el::BooleanFormat::yesNo(), !el::HashSelector{}.isSafe(el::HashAlgorithm::Md5));
}

/// Hash an untrusted stream without collecting it in memory or accepting unlimited input.
auto hashBoundedStream(el::ByteInputStream &input, const el::HashAlgorithm algorithm, const std::size_t maximumBytes)
    -> el::ByteBlock {
    if (!el::HashSelector{}.isSafe(algorithm)) {
        throw el::RuntimeError{"The selected hash algorithm is not acceptable."_el};
    }

    auto hasher = el::Hasher{algorithm};
    auto buffer = std::array<el::Byte, 64U>{};
    auto totalBytes = std::size_t{0U};

    while (true) {
        const auto result = input.read(buffer);
        if (result.isFinished()) {
            break;
        }
        if (result.isTimeout()) {
            throw el::RuntimeError{"Reading the input for hashing timed out."_el};
        }

        const auto blockLength = result.data().toSizeT();
        if (blockLength > maximumBytes - std::min(totalBytes, maximumBytes)) {
            throw el::RuntimeError{"The input exceeds the permitted hashing limit."_el};
        }
        totalBytes += blockLength;
        hasher.update(std::span<const el::Byte>{buffer.data(), blockLength});
    }

    return hasher.finalize();
}

}
