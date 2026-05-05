// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32Format.hpp"

#include "../impl/FormatEngine.hpp"

namespace erbsland::text {

U32Format::U32Format(const std::u32string_view pattern) : U32Format{U32StringView{U32String{pattern}}} {
}

U32Format::U32Format(const U32StringView &pattern) : _data{impl::compileFormat(pattern)} {
}

auto U32Format::fieldCount() const noexcept -> unit::ArgumentCount {
    return _data->fieldCount;
}

}
