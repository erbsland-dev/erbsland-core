// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::text::render::impl {

class Program;
using ProgramPtr = std::shared_ptr<Program>;
using ConstProgramPtr = std::shared_ptr<const Program>;

}
