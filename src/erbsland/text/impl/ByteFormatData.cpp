// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteFormatData.hpp"

#include "../Literals.hpp"

namespace erbsland::text::impl {

using namespace literals;

ByteFormatData::ByteFormatData() : byteSeparator{" "_el}, offsetSeparator{" | "_el}, lineSuffix{"\n"_el} {
}

}
