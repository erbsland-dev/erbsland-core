// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringPatternFactory.hpp"

#include "DynamicStringPatternData.hpp"

#include "../StringCharReader.hpp"

namespace erbsland::text::impl {

auto createStringPatternData(const AnyString &pattern, const StringPatternKind kind) -> StringPatternDataPtr {
    auto reader = StringCharReader{pattern};
    if (kind == StringPatternKind::PosixGlob) {
        return DynamicStringPatternData::parsePosixGlob(reader);
    }
    auto mode = StringPatternParseMode::Divider;
    if (kind == StringPatternKind::Glob) {
        mode = StringPatternParseMode::Glob;
    }
    return DynamicStringPatternData::parse(reader, mode);
}

}
