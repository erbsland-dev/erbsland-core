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

auto StandardStreamProxy::isOpen() const noexcept -> bool {
    return true;
}

void StandardStreamProxy::flush() {
    target()->flush();
}

void StandardStreamProxy::close() {
}

void StandardStreamProxy::write(const text::Char character) {
    target()->write(character);
}

void StandardStreamProxy::write(const text::StringView &text) {
    target()->write(text);
}

void StandardStreamProxy::writeLine() {
    target()->writeLine();
}

void StandardStreamProxy::writeLine(const text::StringView &text) {
    target()->writeLine(text);
}

auto StandardStreamProxy::target() const -> TextOutputStreamPtr {
    return _slot == StandardStreamSlot::Out ? standardStreamRegistry().outputTarget()
                                            : standardStreamRegistry().errorTarget();
}

}
