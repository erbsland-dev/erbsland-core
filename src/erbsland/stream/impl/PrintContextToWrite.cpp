// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PrintContextToWrite.hpp"

#include "../TextOutputStream.hpp"

namespace erbsland::stream::impl {

void PrintContextToWrite::commit() {
    if (!_builder.isEmpty()) {
        _output.write(_builder.takeU8String());
    }
}

}
