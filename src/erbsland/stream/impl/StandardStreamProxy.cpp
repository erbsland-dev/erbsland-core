// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardStreamProxy.hpp"

#include "StandardStreamRegistry.hpp"

namespace erbsland::stream::impl {

StandardStreamProxy::StandardStreamProxy(const StandardStreamSlot slot) : _slot{slot} {
}

auto StandardStreamProxy::encoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto StandardStreamProxy::effectiveEncoding() const noexcept -> text::StringEncoding {
    return text::StringEncoding::Utf8;
}

auto StandardStreamProxy::outputSettings() const noexcept -> const OutputStreamSettings & {
    return target()->outputSettings();
}

auto StandardStreamProxy::state() const noexcept -> StreamState {
    return target()->state();
}

auto StandardStreamProxy::isReady() const noexcept -> bool {
    return target()->isReady();
}

auto StandardStreamProxy::waitForReady() -> StreamWaitStatus {
    return target()->waitForReady();
}

auto StandardStreamProxy::flush() -> StreamWriteStatus {
    return target()->flush();
}

auto StandardStreamProxy::close() -> StreamCloseStatus {
    return StreamCloseStatus::Closed;
}

void StandardStreamProxy::abort() noexcept {
}

auto StandardStreamProxy::write(const text::Char character) -> StreamWriteStatus {
    return target()->write(character);
}

auto StandardStreamProxy::write(const text::StringView &text) -> StreamWriteStatus {
    return target()->write(text);
}

auto StandardStreamProxy::writeLine() -> StreamWriteStatus {
    return target()->writeLine();
}

auto StandardStreamProxy::writeLine(const text::StringView &text) -> StreamWriteStatus {
    return target()->writeLine(text);
}

auto StandardStreamProxy::target() const -> TextOutputStreamPtr {
    return _slot == StandardStreamSlot::Out ? standardStreamRegistry().outputTarget()
                                            : standardStreamRegistry().errorTarget();
}

}
