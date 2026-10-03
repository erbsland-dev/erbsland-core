// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StandardInputStreamProxy.hpp"

#include "StandardStreamRegistry.hpp"

namespace erbsland::stream::impl {

using namespace text;

auto StandardInputStreamProxy::encoding() const noexcept -> StringEncoding {
    return StringEncoding::Utf8;
}

auto StandardInputStreamProxy::effectiveEncoding() const noexcept -> StringEncoding {
    return StringEncoding::Utf8;
}

auto StandardInputStreamProxy::inputSettings() const noexcept -> const InputStreamSettings & {
    return target()->inputSettings();
}

auto StandardInputStreamProxy::state() const noexcept -> StreamState {
    return target()->state();
}

auto StandardInputStreamProxy::isReady() const noexcept -> bool {
    return target()->isReady();
}

auto StandardInputStreamProxy::waitForReady() -> StreamWaitStatus {
    return target()->waitForReady();
}

auto StandardInputStreamProxy::close() -> StreamCloseStatus {
    return StreamCloseStatus::Closed;
}

void StandardInputStreamProxy::abort() noexcept {
}

auto StandardInputStreamProxy::readChar() -> StreamReadResult<Char> {
    return target()->readChar();
}

auto StandardInputStreamProxy::read(const unit::CpCount maximum) -> StreamReadResult<String> {
    return target()->read(maximum);
}

auto StandardInputStreamProxy::readLine(const unit::CpCount maximum) -> StreamReadResult<String> {
    return target()->readLine(maximum);
}

auto StandardInputStreamProxy::readAll(const unit::CpCount maximum) -> StreamReadResult<String> {
    return target()->readAll(maximum);
}

auto StandardInputStreamProxy::target() const -> TextInputStreamPtr {
    return standardStreamRegistry().inputTarget();
}

}
