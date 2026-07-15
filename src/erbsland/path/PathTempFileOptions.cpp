// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathTempFileOptions.hpp"

#include "../text/Literals.hpp"

namespace erbsland::path {

using namespace text::literals;

PathTempFileOptions::PathTempFileOptions() : _prefix{"tmp-"_el} {
}

}
