// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::text::render::impl {

class CompiledBlock;
using ConstCompiledBlockPtr = std::shared_ptr<const CompiledBlock>;

}
