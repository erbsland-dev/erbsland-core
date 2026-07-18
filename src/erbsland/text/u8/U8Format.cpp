// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U8Format.hpp"

#include "../impl/FormatEngine.hpp"

namespace erbsland::text {

U8Format::U8Format(const std::string_view pattern) : U8Format{U8String{pattern}} {
}

U8Format::U8Format(const U8String &pattern) : _data{impl::compileFormat(pattern)} {
}

auto U8Format::fieldCount() const noexcept -> unit::ArgumentCount {
    return _data->fieldCount;
}

}
