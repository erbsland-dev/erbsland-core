// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class Source;
using SourcePtr = std::shared_ptr<Source>;
using SourcePtrConst = std::shared_ptr<const Source>;

}
