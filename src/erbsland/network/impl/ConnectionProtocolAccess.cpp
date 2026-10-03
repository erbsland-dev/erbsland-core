// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ConnectionProtocolAccess.hpp"

namespace erbsland::network::impl {

auto ConnectionProtocolAccess::claim(std::shared_ptr<void> owner) -> bool {
    if (_connection._exclusiveProtocolOwner) {
        return false;
    }
    _connection._exclusiveProtocolOwner = std::move(owner);
    return true;
}

}
