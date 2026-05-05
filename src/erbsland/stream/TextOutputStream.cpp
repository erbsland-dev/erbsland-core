// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextOutputStream.hpp"

#include "impl/PrintContextToWrite.hpp"

namespace erbsland::stream {

auto TextOutputStream::createPrintContext() -> TextPrintContextPtr {
    return std::make_unique<impl::PrintContextToWrite>(*this);
}

}
