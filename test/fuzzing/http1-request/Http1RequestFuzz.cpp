// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/core/Application.hpp>
#include <erbsland/err/Exception.hpp>
#include <erbsland/mem/ByteSpan.hpp>
#include <erbsland/network/impl/http/codec/Http1RequestDecoder.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t *data, const std::size_t size) -> int {
    static const auto application = erbsland::core::Application{};
    static_cast<void>(application);
    if (size < 2U || size > 128U * 1024U) {
        return 0;
    }
    try {
        auto decoder = erbsland::network::impl::Http1RequestDecoder{};
        const auto maximumFragment = std::size_t{1U + data[0] % 64U};
        auto position = std::size_t{1U};
        while (position < size) {
            const auto length = std::min(maximumFragment, size - position);
            const auto fragment = std::span<const uint8_t>{data + position, length};
            if (!decoder.feed(erbsland::mem::toConstByteSpan(fragment)).isAccepted()) {
                break;
            }
            while (decoder.next().has_value()) {
            }
            position += length;
        }
        decoder.endOfInput();
        while (decoder.next().has_value()) {
        }
    } catch (const erbsland::err::Exception &) {
        // Arbitrary syntax, framing, limits, EOF, and lifecycle failures are expected fuzz outcomes.
    }
    return 0;
}
