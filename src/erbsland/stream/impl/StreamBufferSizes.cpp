// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamBufferSizes.hpp"

#include <cstddef>

namespace erbsland::stream::impl {

using unit::ByteCount;

auto streamBufferSizes(const StreamBuffering buffering) noexcept -> StreamBufferSizes {
    constexpr auto kib = std::size_t{1024U};
    constexpr auto mib = std::size_t{1024U * 1024U};
    switch (buffering) {
    case StreamBuffering::MinimalMemory:
        return {
            ByteCount{4U * kib}, ByteCount{16U * kib}, ByteCount{4U * kib}, ByteCount{4U * kib}, ByteCount{256U * kib}};
    case StreamBuffering::Interactive:
        return {
            ByteCount{16U * kib}, ByteCount{32U * kib}, ByteCount{16U * kib}, ByteCount{4U * kib}, ByteCount{1U * mib}};
    case StreamBuffering::Balanced:
        return {
            ByteCount{64U * kib},
            ByteCount{64U * kib},
            ByteCount{64U * kib},
            ByteCount{16U * kib},
            ByteCount{16U * mib}};
    case StreamBuffering::Throughput:
        return {
            ByteCount{256U * kib},
            ByteCount{256U * kib},
            ByteCount{128U * kib},
            ByteCount{64U * kib},
            ByteCount{64U * mib}};
    case StreamBuffering::Bulk:
        return {
            ByteCount{1U * mib},
            ByteCount{1U * mib},
            ByteCount{256U * kib},
            ByteCount{256U * kib},
            ByteCount{256U * mib}};
    }
    return {};
}

}
