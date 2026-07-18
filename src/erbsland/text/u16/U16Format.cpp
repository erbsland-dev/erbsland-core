// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16Format.hpp"

#include "../impl/FormatEngine.hpp"

namespace erbsland::text {

U16Format::U16Format(const std::u16string_view pattern) : U16Format{U16String{pattern}} {
}

U16Format::U16Format(const U16String &pattern) : _data{impl::compileFormat(pattern)} {
}

auto U16Format::fieldCount() const noexcept -> unit::ArgumentCount {
    return _data->fieldCount;
}

}
