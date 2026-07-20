// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Replacement.hpp"

#include "../engine/CaptureGroupNames.hpp"

#include "../../../text/String.hpp"
#include "../../../text/StringCharReader.hpp"
#include "../../../text/StringCIHashMap.hpp"
#include "../../CaptureGroup.hpp"

namespace erbsland::re::impl {

/// A parser for replacement text.
class ReplacementParser {
public:
    ReplacementParser(const text::String &expression, const CaptureGroupNames &groupNames);

public:
    [[nodiscard]] auto parse() -> Replacement;

private:
    void readNext();
    void parseExpression();
    void parseGroupIndex();
    void parseGroupName();
    void requireMoreContent();
    [[noreturn]] void throwError(text::String description) const;

private:
    Replacement _replacement;
    text::StringCharReader _reader;
    text::Char _currentChar{text::Char::noCodePoint()};
    text::StringCIHashMap<CaptureGroupIndex> _nameMap;
    text::StringEditor _staticText;
};

}
