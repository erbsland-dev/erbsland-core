// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FormatEngine.hpp"

#include "FormatParser.hpp"
#include "FormatWriter.hpp"
#include "ThrowHelper.hpp"

#include "../Literals.hpp"

namespace erbsland::text::impl {

using namespace erbsland::text::literals;

auto compileFormat(const U8String &pattern) -> FormatDataPtr {
    return FormatParser{pattern}.parse();
}

auto compileFormat(const U16String &pattern) -> FormatDataPtr {
    return FormatParser{pattern}.parse();
}

auto compileFormat(const U32String &pattern) -> FormatDataPtr {
    return FormatParser{pattern}.parse();
}

auto appendFormat(const FormatData &format, AnyStringBuilder &builder, const std::span<const FormatArgument> arguments)
    -> AnyStringBuilder & {
    if (arguments.size() != format.argumentCount.toSizeT()) {
        text::impl::throwFormatError("Format argument count does not match the pattern"_el);
    }

    auto writer = FormatWriter{builder};
    for (const auto &part : format.parts) {
        if (part.kind() == FormatPartKind::StaticText) {
            writer.appendField(part, {});
            continue;
        }
        const auto &argument = arguments[part.argumentIndex().toSizeT()];
        writer.appendField(part, argument);
    }
    return builder;
}

}
