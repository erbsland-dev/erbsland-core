// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringSource.hpp"

#include "../constants/Limits.hpp"

#include "../../../text/StringFormat.hpp"
#include "../../../text/StringList.hpp"
#include "../../../unit/LineCount.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

StringSource::StringSource(text::String text) noexcept :
    _lineSplitter{std::move(text), text::Char{U'\n'}, text::StringSplitMode::KeepSeparator} {
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

auto StringSource::readLine() -> text::String {
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

auto StringSource::codeSnippet(const unit::CodeLocation location) noexcept -> std::optional<text::CodeSnippet> {
    const auto &text = _lineSplitter.text();
    if (location.line().isNoIndex() || text.isEmpty()) {
        return std::nullopt;
    }
    static const auto newLineOrCR = text::CharSet{"\r\n"_el};
    const auto firstLine = location.line().retreated(unit::LineCount{2U});
    const auto lastLine = location.line().advanced(unit::LineCount{2U});
    auto splitter = text::StringSplitter{text, text::Char{U'\n'}, text::StringSplitMode::KeepSeparator};
    auto lineIndex = unit::LineIndex::zero();
    while (lineIndex < firstLine) {
        if (splitter.isAtEnd()) {
            return std::nullopt;
        }
        static_cast<void>(splitter.next());
        ++lineIndex;
    }

    auto lines = text::StringList{};
    auto found = false;
    while (lineIndex <= lastLine && !splitter.isAtEnd()) {
        auto line = splitter.next().trimmed(newLineOrCR, text::StringSide::Back);
        lines.append(std::move(line));
        found = found || lineIndex == location.line();
        ++lineIndex;
    }
    if (!found) {
        return std::nullopt;
    }
    return text::CodeSnippet{std::move(lines), firstLine, "elcl"_el};
}

void StringSource::throwLineLengthExceeded() {
    close();
    throw ConfError(
        ConfErrorCategory::LimitExceeded,
        text::StringFormat{"The line exceeds the maximum size of {} bytes."_el}.build(limits::maxLineLength),
        Location{identifier()});
}

}
