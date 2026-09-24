// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::compression::impl {

class CompressionCodec;
/// Shared ownership pointer for compression codec instances.
using CompressionCodecPtr = std::shared_ptr<CompressionCodec>;

}
