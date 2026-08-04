// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TlsRecordPlaintext.hpp"

#include <utility>

namespace erbsland::cryptology {

TlsRecordPlaintext::TlsRecordPlaintext(const TlsRecordContentType type, mem::ByteBlock content) noexcept :
    _type{type}, _content{std::move(content)} {
    _content.markAsSensitive();
}

TlsRecordPlaintext::~TlsRecordPlaintext() {
    secureErase();
}

void TlsRecordPlaintext::secureErase() noexcept {
    _content.secureErase();
    _content = {};
}

auto TlsRecordPlaintext::takeContent() noexcept -> mem::ByteBlock {
    return std::exchange(_content, {});
}

}
