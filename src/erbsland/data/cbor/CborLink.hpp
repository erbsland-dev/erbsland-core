// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"

namespace erbsland::data::cbor {
/// Opaque binary CID carried by DAG-CBOR tag 42.
/// @tested{CborValueTest}
struct CborLink final {
    mem::ByteBlock cid; ///< CID bytes without the wire prefix.
};
}
