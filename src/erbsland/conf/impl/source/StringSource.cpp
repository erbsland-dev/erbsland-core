// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringSource.hpp"

#include "../constants/Limits.hpp"

#include "../../../text/StringFormat.hpp"
#include "../../../text/StringList.hpp"
#include "../../../unit/LineCount.hpp"

namespace erbsland::conf::impl {

using namespace text;
using namespace unit;
using namespace text::literals;

StringSource::StringSource(String text) noexcept :
    _lineSplitter{std::move(text), Char{U'\n'}, StringSplitMode::KeepSeparator} {
}

auto StringSource::identifier() const noexcept -> SourceIdentifierPtr {
    static auto identifier = SourceIdentifier::createForText();
    return identifier;
}

void StringSource::open() {
    if (_isOpen) {
        throw ConfError(ConfErrorCategory::Internal, "The source is already open."_el, Location{identifier()});
    }
    _lineSplitter.reset();
    _isOpen = true;
    _atEnd = false;
}

auto StringSource::readLine() -> String {
    if (_atEnd) {
        return {};
    }
    if (!_isOpen) {
        throw ConfError(ConfErrorCategory::IO, "You cannot read from a closed source."_el, Location{identifier()});
    }

    auto result = _lineSplitter.next();
    if (_lineSplitter.isAtEnd()) {
        _atEnd = true;
        _isOpen = false;
    }
    if (result.length().toSizeT() > limits::maxLineLength) {
        throwLineLengthExceeded();
    }
    return result;
}

auto StringSource::codeSnippet(const CodeLocation location) noexcept -> std::optional<CodeSnippet> {
    const auto &text = _lineSplitter.text();
    if (location.line().isNoIndex() || text.isEmpty()) {
        return std::nullopt;
    }
    static const auto newLineOrCR = CharSet{"\r\n"_el};
    const auto firstLine = location.line().retreated(LineCount{2U});
    const auto lastLine = location.line().advanced(LineCount{2U});
    auto splitter = StringSplitter{text, Char{U'\n'}, StringSplitMode::KeepSeparator};
    auto lineIndex = LineIndex::zero();
    while (lineIndex < firstLine) {
        if (splitter.isAtEnd()) {
            return std::nullopt;
        }
        splitter.skip();
        ++lineIndex;
    }

    auto lines = StringList{};
    auto found = false;
    while (lineIndex <= lastLine && !splitter.isAtEnd()) {
        auto line = splitter.next().trimmed(newLineOrCR, StringSide::Back);
        lines.append(std::move(line));
        found = found || lineIndex == location.line();
        ++lineIndex;
    }
    if (!found) {
        return std::nullopt;
    }
    return CodeSnippet{std::move(lines), firstLine, "elcl"_el};
}

void StringSource::throwLineLengthExceeded() {
    close();
    throw ConfError(
        ConfErrorCategory::LimitExceeded,
        StringFormat{"The line exceeds the maximum size of {} bytes."_el}.build(limits::maxLineLength),
        Location{identifier()});
}

}
