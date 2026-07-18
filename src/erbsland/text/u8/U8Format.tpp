// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../impl/FormatEngine.hpp"

namespace erbsland::text {

template <typename... Args>
auto U8Format::build(Args &&...args) const -> U8String {
    auto builder = AnyStringBuilder{StringKind::U8};
    appendTo(builder, std::forward<Args>(args)...);
    return builder.takeU8String();
}

template <typename... Args>
auto U8Format::appendTo(AnyStringBuilder &builder, Args &&...args) const -> AnyStringBuilder & {
    const auto arguments = impl::makeFormatArguments(std::forward<Args>(args)...);
    return impl::appendFormat(*_data, builder, std::span<const FormatArgument>{arguments});
}

}
