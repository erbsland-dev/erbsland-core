// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <StreamDemoSupport.hpp>

#include <array>
#include <exception>
#include <span>

namespace demo {

class PrimeSequenceCompressor final {
public:
    void compress(const std::span<const el::Byte> bytes) { _inputLength += bytes.size(); }
    void finish() const { el::io::printLine("Compressed input: "_el, _inputLength, " bytes"_el); }

private:
    std::size_t _inputLength{0U};
};

void compressPrimeSequence(el::ByteInputStream &input, PrimeSequenceCompressor &compressor);

void processPayloadInChunks() {
    auto input = ScriptedByteInputStream{{2U, 3U, 5U, 7U, 11U, 13U, 17U}, 3U, 1U};
    auto compressor = PrimeSequenceCompressor{};
    compressPrimeSequence(input, compressor);
}

/// Feed large binary input into a compressor without collecting the complete source in memory.
/// A short `Data` result is normal; only the reported prefix of the buffer contains new input.
void compressPrimeSequence(el::ByteInputStream &input, PrimeSequenceCompressor &compressor) {
    constexpr auto cMaximumConsecutiveTimeouts = 60U;
    auto buffer = std::array<el::Byte, 64U * 1024U>{};
    auto consecutiveTimeouts = 0U;

    try {
        while (true) {
            const auto result = input.read(buffer);
            if (result.isFinished()) {
                break;
            }
            if (result.isTimeout()) {
                if (++consecutiveTimeouts == cMaximumConsecutiveTimeouts) {
                    throw el::RuntimeError{"The prime sequence did not provide data for too long."_el};
                }
                continue;
            }

            consecutiveTimeouts = 0U;
            compressor.compress(std::span<const el::Byte>{buffer.data(), result.data().toSizeT()});
        }
    } catch (const el::StreamError &) {
        throw el::RuntimeError{"The prime sequence could not be compressed."_el, std::current_exception()};
    }
    compressor.finish();
}

}
