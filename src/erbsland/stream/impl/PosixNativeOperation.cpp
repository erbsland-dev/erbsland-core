// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PosixNativeOperation.hpp"

#include "PosixNativeStream.hpp"

#include "../../text/Literals.hpp"

namespace erbsland::stream::impl {

using namespace text::literals;

PosixNativeOperation::PosixNativeOperation(const PosixNativeStream &stream) : _stream{stream} {
    const auto lock = std::scoped_lock{_stream._operationMutex};
    _fileDescriptor = _stream._fileDescriptor.load();
    if (_fileDescriptor < 0) {
        _stream.throwError("Failed to access the native stream."_el, "The POSIX native stream is closed."_el);
    }
    ++_stream._operationCount;
}

PosixNativeOperation::~PosixNativeOperation() {
    _stream.finishOperation();
}

}
