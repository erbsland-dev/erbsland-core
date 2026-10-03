// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "VirtualBackend.hpp"

#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../time/TimeDelta.hpp"

#include <thread>

namespace erbsland::cterm::impl {

VirtualBackend::VirtualBackend() :
    _output{stream::impl::createNativeStandardOutputStream(stream::impl::NativeStandardStream::Out)} {
}

void VirtualBackend::initializePlatform() {
    _initialized = true;
}

void VirtualBackend::restorePlatform() {
    emitFlush();
    _initialized = false;
}

void VirtualBackend::emitText(const text::String &text) {
    _output->writeBytes(text::impl::UnsafeU8StringAccess{text}.dataView().dataSpan());
}

void VirtualBackend::emitFlush() {
    _output->flush();
}

auto VirtualBackend::readKey(const time::Milliseconds timeout) -> Key {
    if (_inputMode == Input::Mode::Key && timeout > time::Milliseconds{}) {
        std::this_thread::sleep_for(time::TimeDelta{timeout}.toStdNanoseconds());
    }
    return {};
}

}
