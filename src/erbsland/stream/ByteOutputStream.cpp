// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteOutputStream.hpp"

#include "../mem/impl/UnsafeByteBlockAccess.hpp"

namespace erbsland::stream {

auto ByteOutputStream::endianness() const noexcept -> mem::Endianness {
    return _endianness;
}

void ByteOutputStream::setEndianness(const mem::Endianness endianness) noexcept {
    _endianness = endianness;
}

auto ByteOutputStream::write(const mem::Byte byte) -> StreamWriteStatus {
    return write(mem::ConstByteSpan{&byte, 1U});
}

auto ByteOutputStream::write(const mem::ByteBlock &bytes) -> StreamWriteStatus {
    return write(mem::impl::UnsafeByteBlockAccess{bytes}.dataView().dataSpan());
}

auto ByteOutputStream::coWrite(mem::ByteBlock bytes) -> util::CoTask<StreamWriteStatus> {
    auto self = std::static_pointer_cast<ByteOutputStream>(sharedOutputStream());
    return util::CoTask<StreamWriteStatus>::run(
        [self = std::move(self), bytes = std::move(bytes)]() -> StreamWriteStatus { return self->write(bytes); });
}

}
