// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/StringPatternFactory.hpp"

namespace erbsland::text {

template <pattern::AnyElement... Args>
StringPattern::StringPattern(const Args &...elements) : _data{impl::createStaticStringPatternData(elements...)} {
}

}
