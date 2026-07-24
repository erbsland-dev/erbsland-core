// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamBufferSizes.hpp"

#include <cstddef>

namespace erbsland::stream::impl {

using unit::ByteLength;

auto streamBufferSizes(const StreamBuffering buffering) noexcept -> StreamBufferSizes {
    constexpr auto kib = std::size_t{1024U};
    constexpr auto mib = std::size_t{1024U * 1024U};
    switch (buffering) {
    case StreamBuffering::MinimalMemory:
        return {
            ByteLength{4U * kib},
            ByteLength{16U * kib},
            ByteLength{4U * kib},
            ByteLength{4U * kib},
            ByteLength{256U * kib}};
    case StreamBuffering::Interactive:
        return {
            ByteLength{16U * kib},
            ByteLength{32U * kib},
            ByteLength{16U * kib},
            ByteLength{4U * kib},
            ByteLength{1U * mib}};
    case StreamBuffering::Balanced:
        return {
            ByteLength{64U * kib},
            ByteLength{64U * kib},
            ByteLength{64U * kib},
            ByteLength{16U * kib},
            ByteLength{16U * mib}};
    case StreamBuffering::Throughput:
        return {
            ByteLength{256U * kib},
            ByteLength{256U * kib},
            ByteLength{128U * kib},
            ByteLength{64U * kib},
            ByteLength{64U * mib}};
    case StreamBuffering::Bulk:
        return {
            ByteLength{1U * mib},
            ByteLength{1U * mib},
            ByteLength{256U * kib},
            ByteLength{256U * kib},
            ByteLength{256U * mib}};
    }
    return {};
}

}
