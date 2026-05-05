// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixFileDescriptor.hpp"

#include "../../err/RandomError.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace erbsland::random::impl {

PosixFileDescriptor::PosixFileDescriptor(const std::string &path) {
    _descriptor = ::open(path.c_str(), O_RDONLY);
    if (_descriptor < 0) {
        throw err::RandomError{"Cannot open system entropy source"};
    }
}

PosixFileDescriptor::~PosixFileDescriptor() {
    if (_descriptor >= 0) {
        ::close(_descriptor);
    }
}

}
