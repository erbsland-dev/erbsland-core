// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Borrow a short-lived view of a local byte array.
///
/// `FixedByteSpan` and `FixedConstByteSpan` preserve a known extent in the
/// type, while `ByteSpan` and `ConstByteSpan` accept a size determined at
/// runtime. All four are non-owning views, so the referenced storage must stay
/// alive and in place for the complete use of the span.
void borrowedViews() {
    const auto averageSignal = [](const el::ConstByteSpan samples) -> uint32_t {
        auto sum = uint32_t{};
        for (const auto sample : samples) {
            sum += sample.toUInt32();
        }
        return samples.empty() ? 0U : sum / static_cast<uint32_t>(samples.size());
    };

    auto observation =
        std::array{el::Byte{12U}, el::Byte{21U}, el::Byte{34U}, el::Byte{55U}, el::Byte{89U}, el::Byte{144U}};

    // Use a writable fixed span while the six-pixel extent is part of the algorithm.
    auto fixedPixels = el::FixedByteSpan<6>{observation};
    fixedPixels.front() = el::Byte{13U};

    // A dynamic span is useful when the relevant range is chosen at runtime.
    const auto visiblePixelCount = std::size_t{4U};
    auto visiblePixels = el::ByteSpan{fixedPixels}.first(visiblePixelCount);
    visiblePixels.back() = el::Byte{56U};

    // Read-only spans document that the receiving function cannot change the bytes.
    const auto corePixels = el::FixedConstByteSpan<4>{fixedPixels.first<4>()};
    const auto average = averageSignal(el::ConstByteSpan{corePixels});

    el::io::printLine("Observation        : Νεφέλωμα του Ωρίωνα"_el);
    el::io::printLine("Captured pixels    : "_el, fixedPixels.size());
    el::io::printLine("Visible pixels     : "_el, visiblePixels.size());
    el::io::printLine("Average signal     : "_el, average);
}

}
