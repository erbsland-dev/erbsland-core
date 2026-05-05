// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreams.hpp"

#include "impl/NativeOutputStream.hpp"
#include "impl/StandardTextOutputStream.hpp"

#include <memory>

namespace erbsland::stream {

auto stdOut() -> TextOutputStreamPtr {
    static const auto outputStream = std::make_shared<impl::StandardTextOutputStream>(
        impl::createNativeStandardOutputStream(impl::NativeStandardStream::Out));
    return outputStream;
}

auto stdErr() -> TextOutputStreamPtr {
    static const auto errorStream = std::make_shared<impl::StandardTextOutputStream>(
        impl::createNativeStandardOutputStream(impl::NativeStandardStream::Err));
    return errorStream;
}

}
