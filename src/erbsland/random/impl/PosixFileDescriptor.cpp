// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixFileDescriptor.hpp"

#include "../RandomError.hpp"

#include "../../system/PlatformError.hpp"
#include "../../system/PosixErrorContext.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringEditor.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace erbsland::random::impl {

using namespace text::literals;

PosixFileDescriptor::PosixFileDescriptor(const std::string &path) {
    _descriptor = ::open(path.c_str(), O_RDONLY);
    if (_descriptor < 0) {
        const auto errorCode = errno;
        auto cause = std::make_exception_ptr(
            system::PlatformError{
                "Cannot open system entropy source"_el, system::PosixErrorContext::fromErrorCode(errorCode)});
        throw random::RandomError{"Cannot open system entropy source"_el, std::move(cause)};
    }
}

PosixFileDescriptor::~PosixFileDescriptor() {
    if (_descriptor >= 0) {
        ::close(_descriptor);
    }
}

}
