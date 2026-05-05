// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixEntropySource.hpp"

#include "PosixFileDescriptor.hpp"

#include "../../err/RandomError.hpp"

#include <unistd.h>

#include <cerrno>
#include <utility>

namespace erbsland::random::impl {

PosixEntropySource::PosixEntropySource(std::string path) : _path{std::move(path)} {
}

void PosixEntropySource::fillBytes(const std::span<std::byte> destination) {
    if (destination.empty()) {
        return;
    }

    const auto source = PosixFileDescriptor{_path};
    auto *data = reinterpret_cast<unsigned char *>(destination.data());
    auto remaining = destination.size();
    while (remaining > 0U) {
        const auto readCount = ::read(source.descriptor(), data, remaining);
        if (readCount < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw err::RandomError{"System entropy source failed"};
        }
        if (readCount == 0) {
            throw err::RandomError{"System entropy source ended unexpectedly"};
        }
        data += readCount;
        remaining -= static_cast<std::size_t>(readCount);
    }
}

}
