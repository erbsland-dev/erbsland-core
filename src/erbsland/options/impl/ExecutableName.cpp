// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ExecutableName.hpp"

#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteRange.hpp"

namespace erbsland::options::impl {

using namespace text::literals;

auto extractExecutableName(const text::StringView &executablePath) -> text::String {
    auto executableName = executablePath;
    auto separatorIndex = executableName.findLastOf(text::CharSet{"/\\"_el});
    if (!separatorIndex.isNoIndex()) {
        executableName.advance(separatorIndex);
        executableName = executablePath.slice(unit::ByteRange{separatorIndex, unit::ByteLength::infinite()});
    }
    constexpr auto suffix = ".exe"_el;
    if (executableName.endsWith(suffix, text::Char::compareAsciiFolded)) {
        executableName =
            executableName.slice(unit::ByteRange{unit::ByteIndex::zero(), executableName.length() - suffix.length()});
    }
    return text::String{executableName};
}

}
