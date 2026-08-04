// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HmacWorkerFactory.hpp"

#include "HmacWorkerAdapter.hpp"

#include "../algorithm/Sha2.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../text/Literals.hpp"

#include <cstdint>
#include <limits>
#include <memory>

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto isHmacAlgorithmSupported(const HashAlgorithm algorithm) noexcept -> bool {
    return algorithm == HashAlgorithm::Sha2_256 || algorithm == HashAlgorithm::Sha2_384;
}

auto createHmacWorker(const HashAlgorithm algorithm, const mem::ConstByteSpan key) -> std::unique_ptr<HmacWorker> {
    if (algorithm == HashAlgorithm::Sha2_256) {
        // FIPS 180-4 section 6.2 gives SHA-256 a 64-byte block and a 64-bit bit-length field. RFC 2104 preloads one
        // complete block for K0 xor ipad, leaving floor((2^64 - 1) / 8) - 64 message bytes.
        constexpr auto maximumMessageLength = (uint64_t{1U} << 61U) - 65U;
        using Worker = HmacWorkerAdapter<Sha2_256, HashAlgorithm::Sha2_256, 64U, maximumMessageLength>;
        return std::make_unique<Worker>(key);
    }
    if (algorithm == HashAlgorithm::Sha2_384) {
        // FIPS 180-4 section 6.5 gives SHA-384 a 128-byte block and a 128-bit bit-length field. The portable facade's
        // uint64_t byte counter is the tighter bound, and overflow is rejected before the underlying hash is updated.
        using Worker = HmacWorkerAdapter<Sha2_384, HashAlgorithm::Sha2_384, 128U, std::numeric_limits<uint64_t>::max()>;
        return std::make_unique<Worker>(key);
    }
    throw err::ParameterError{"HMAC supports only SHA-256 and SHA-384."_el, "algorithm"_el};
}

}
