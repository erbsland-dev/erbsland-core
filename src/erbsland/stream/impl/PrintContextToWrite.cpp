// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PrintContextToWrite.hpp"

#include "../TextOutputStream.hpp"

namespace erbsland::stream::impl {

auto PrintContextToWrite::commit() -> StreamWriteStatus {
    if (!_builder.isEmpty()) {
        return _output.write(_builder.takeU8String());
    }
    return StreamWriteStatus::Success;
}

}
