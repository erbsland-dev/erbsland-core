// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "NativeOutputStream.hpp"

#include "../../text/impl/UnsafeU8StringViewAccess.hpp"

namespace erbsland::stream::impl {

void NativeOutputStream::writeText(const text::StringView &text) {
    const auto dataView = text::impl::UnsafeU8StringViewAccess{text}.dataView();
    const auto dataSpan = dataView.dataSpan();
    if (dataSpan.empty()) {
        return;
    }
    writeBytes(dataSpan);
}

}
