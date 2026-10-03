// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteOutputStream.hpp"

#include "../mem/impl/UnsafeByteBlockAccess.hpp"

namespace erbsland::stream {

using namespace mem;

auto ByteOutputStream::endianness() const noexcept -> Endianness {
    return _endianness;
}

void ByteOutputStream::setEndianness(const Endianness endianness) noexcept {
    _endianness = endianness;
}

auto ByteOutputStream::write(const Byte byte) -> StreamWriteStatus {
    return write(ConstByteSpan{&byte, 1U});
}

auto ByteOutputStream::write(const ByteBlock &bytes) -> StreamWriteStatus {
    return write(mem::impl::UnsafeByteBlockAccess{bytes}.dataView().dataSpan());
}

auto ByteOutputStream::coWrite(ByteBlock bytes) -> util::CoTask<StreamWriteStatus> {
    auto self = std::static_pointer_cast<ByteOutputStream>(sharedOutputStream());
    return util::CoTask<StreamWriteStatus>::run(
        [self = std::move(self), bytes = std::move(bytes)]() -> StreamWriteStatus { return self->write(bytes); });
}

}
