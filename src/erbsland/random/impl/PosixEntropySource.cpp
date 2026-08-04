// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixEntropySource.hpp"

#include "PosixFileDescriptor.hpp"

#include "../RandomError.hpp"

#include "../../system/PlatformError.hpp"
#include "../../system/PosixErrorContext.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"

#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <utility>

namespace erbsland::random::impl {

using namespace text::literals;

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
            const auto errorCode = errno;
            throw system::PlatformError{
                "System entropy source failed"_el, system::PosixErrorContext::fromErrorCode(errorCode)};
        }
        if (readCount == 0) {
            throw system::PlatformError{"System entropy source ended unexpectedly"_el};
        }
        data += readCount;
        remaining -= static_cast<std::size_t>(readCount);
    }
}

}
