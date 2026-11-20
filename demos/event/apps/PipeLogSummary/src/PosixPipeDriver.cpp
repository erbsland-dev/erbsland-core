// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixPipeDriver.hpp"

#include <erbsland/ApplicationError.hpp>
#include <erbsland/PlatformError.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/system/PlatformFailure.hpp>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <utility>

namespace demo {

using namespace el::text::literals;

PosixPipeDriver::PosixPipeDriver(const std::optional<el::Path> &file) : _input{STDIN_FILENO} {
    if (file) {
        _file.reset(std::fopen(file->toStdPath().c_str(), "rb"));
        if (!_file) {
            throw el::PlatformError{
                el::StringFormat{"Cannot open {} (errno {})."_el}.build(file->toNativeString(), errno)};
        }
        _input = ::fileno(_file.get());
    } else if (::isatty(_input)) {
        throw el::ApplicationError{"Pipe a log into standard input or supply a file name."_el};
    }
    if (::pipe(_wakePipe.data()) < 0) {
        throw el::PlatformError{el::StringFormat{"Cannot create wake pipe (errno {})."_el}.build(errno)};
    }
    for (const auto fd : _wakePipe) {
        if (::fcntl(fd, F_SETFL, O_NONBLOCK) < 0 || ::fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
            const auto error = errno;
            ::close(_wakePipe[0]);
            ::close(_wakePipe[1]);
            throw el::PlatformError{el::StringFormat{"Cannot configure wake pipe (errno {})."_el}.build(error)};
        }
    }
}

PosixPipeDriver::~PosixPipeDriver() {
    ::close(_wakePipe[0]);
    ::close(_wakePipe[1]);
}

void PosixPipeDriver::wait() {
    pollOnce(-1);
}

void PosixPipeDriver::wait(const el::TimeDelta maximumWait) {
    if (!maximumWait.isPositive()) {
        pollOnce(0);
        return;
    }
    const auto ns = maximumWait.toNanoseconds().toRawValue();
    const auto milliseconds = 1 + (ns - 1) / 1'000'000;
    pollOnce(static_cast<int>(std::min<int64_t>(milliseconds, INT_MAX)));
}

void PosixPipeDriver::wake() {
    const char byte = 1;
    while (::write(_wakePipe[1], &byte, 1) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return; // A full pipe is already readable.
        }
        if (errno != EINTR) {
            throw el::system::PlatformFailure{
                el::StringFormat{"Cannot wake the log reader (errno {})."_el}.build(errno)};
        }
    }
}

void PosixPipeDriver::pollOnce(const int timeout) {
    pollfd descriptors[] = {{_wakePipe[0], POLLIN, 0}, {_input, POLLIN, 0}};
    if (::poll(descriptors, 2, timeout) < 0) {
        if (errno == EINTR) {
            return; // Let the loop recompute the remaining deadline.
        }
        throw el::PlatformError{el::StringFormat{"Polling log input failed (errno {})."_el}.build(errno)};
    }
    if (descriptors[0].revents & POLLIN) {
        std::array<char, 256> bytes;
        if (::read(_wakePipe[0], bytes.data(), bytes.size()) < 0 && errno != EINTR) {
            throw el::PlatformError{"Cannot consume a wake notification."_el};
        }
    }
    if (descriptors[1].revents & (POLLERR | POLLNVAL)) {
        throw el::PlatformError{"The log input descriptor failed."_el};
    }
    if (descriptors[1].revents & (POLLIN | POLLHUP)) {
        _inputReady = true; // A hangup can still have buffered data.
    }
}

auto PosixPipeDriver::readReadyChunk() -> std::optional<el::ByteBlock> {
    if (!std::exchange(_inputReady, false)) {
        return {};
    }
    std::array<char, 4096> bytes;
    const auto count = ::read(_input, bytes.data(), bytes.size());
    if (count < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return {};
        }
        throw el::PlatformError{el::StringFormat{"Reading log input failed (errno {})."_el}.build(errno)};
    }
    if (count == 0) {
        _input = -1; // poll ignores this slot from now on.
    }
    return el::ByteBlock::fromSpan(std::span{bytes}.first(static_cast<std::size_t>(count)));
}

}
