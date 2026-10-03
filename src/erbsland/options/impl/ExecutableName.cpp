// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ExecutableName.hpp"

#include "../../text/CharSet.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteRange.hpp"

namespace erbsland::options::impl {

using namespace text;
using namespace text::literals;

auto extractExecutableName(const String &executablePath) -> String {
    static const auto separatorCharacters = CharSet{"/\\"_el};
    auto executableName = executablePath;
    if (const auto parts = executablePath.splitAtLast(separatorCharacters)) {
        executableName = parts->second;
    }
    constexpr auto suffix = ".exe"_el;
    if (executableName.endsWith(suffix, Char::compareAsciiFolded)) {
        executableName =
            executableName.slice(unit::ByteRange{unit::ByteIndex::zero(), executableName.length() - suffix.length()});
    }
    return executableName.copy();
}

}
