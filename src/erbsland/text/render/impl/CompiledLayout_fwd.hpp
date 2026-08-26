// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::text::render::impl {

class CompiledLayout;
using CompiledLayoutPtr = std::shared_ptr<CompiledLayout>;
using ConstCompiledLayoutPtr = std::shared_ptr<const CompiledLayout>;

}
