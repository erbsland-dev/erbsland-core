// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <memory>

namespace erbsland::re::impl {

class Program;
using ProgramPtr = std::shared_ptr<Program>;
using ConstProgramPtr = std::shared_ptr<const Program>;
using ProgramCounter = uint16_t;

}
