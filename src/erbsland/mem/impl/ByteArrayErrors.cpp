// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteArrayErrors.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::mem::impl {

using namespace text::literals;

void throwByteArrayWrongLength() {
    throw err::ParameterError{"The byte span has the wrong length."_el, "bytes"_el};
}

}
