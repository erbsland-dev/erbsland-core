// Copyright (c) 2024-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "TextStreamSource.hpp"

#include "../constants/Limits.hpp"

#include "../../../path/PathError.hpp"
#include "../../../stream/StreamError.hpp"
#include "../../../text/EncodingError.hpp"
#include "../../../text/StringFormat.hpp"
#include "../../../unit/LineCount.hpp"

#include <algorithm>

namespace erbsland::conf::impl {

using namespace text;
using namespace unit;
using namespace text::literals;

void TextStreamSource::open() {
    if (_isOpen) {
        throw ConfError(ConfErrorCategory::Internal, "The source is already open."_el, Location{identifier()});
    }
    _recentLines.clear();
    _nextLine = LineIndex::zero();
    _atEnd = false;
    try {
        _stream = createStream();
        if (!_stream || !_stream->isOpen()) {
            throw ConfError(ConfErrorCategory::IO, "Failed to open the source stream."_el, Location{identifier()});
        }
        _streamSupportsPositioning = _stream->supportsPositioning();
        _isOpen = true;
    } catch (const ConfError &) {
        throw;
    } catch (const stream::StreamError &) {
        const auto cause = std::current_exception();
        close();
        throw ConfError{
            ConfErrorCategory::IO,
            "Opening the Configuration Source Failed"_el,
            "The source stream could not be opened."_el,
            Location{identifier()},
            cause};
    } catch (const path::PathError &) {
        const auto cause = std::current_exception();
        close();
        throw ConfError{
            ConfErrorCategory::IO,
            "Opening the Configuration Source Failed"_el,
            "The source path could not be opened for reading."_el,
            Location{identifier()},
            cause};
    }
}

auto TextStreamSource::readLine() -> String {
    if (_atEnd) {
        return {};
    }
    if (!_isOpen) {
        throw ConfError(ConfErrorCategory::IO, "You cannot read from a closed source."_el, Location{identifier()});
    }
    try {
        auto linePosition = std::optional<ByteIndex>{};
        if (_streamSupportsPositioning) {
            linePosition = _stream->position();
        }
        auto line = readStreamLine();
        if (!line) {
            sourceIsAtEnd();
            return {};
        }
        if (line->length().toSizeT() > limits::maxLineLength) {
            throwLineLengthExceeded();
        }
        rememberLine(*line, linePosition);
        return std::move(*line);
    } catch (const ConfError &) {
        throw;
    } catch (const EncodingError &) {
        throwReadError(
            ConfErrorCategory::Encoding,
            "Decoding the Configuration Source Failed"_el,
            "The input stream contains invalid encoded text."_el);
    } catch (const stream::StreamError &) {
        throwReadError(
            ConfErrorCategory::IO,
            "Reading the Configuration Source Failed"_el,
            "The input stream could not be read."_el);
    } catch (const path::PathError &) {
        throwReadError(
            ConfErrorCategory::IO,
            "Reading the Configuration Source Failed"_el,
            "The source path could not be read."_el);
    }
}

auto TextStreamSource::codeSnippet(const CodeLocation location) noexcept -> std::optional<CodeSnippet> {
    static const auto lineBreakCharacters = CharSet{"\r\n"_el};
    if (location.line().isNoIndex() || _recentLines.empty()) {
        return std::nullopt;
    }
    const auto requestedLine = location.line();
    const auto hasRequestedLine = std::ranges::any_of(
        _recentLines, [requestedLine](const auto &entry) noexcept { return entry.first == requestedLine; });
    if (!hasRequestedLine) {
        return std::nullopt;
    }

    const auto maximumLine = requestedLine.advanced(LineCount{2U});
    auto additionalLineCount = LineCount::zero();
    while (
        _isOpen && !_atEnd && _stream != nullptr && _stream->isReady() && _nextLine <= maximumLine &&
        additionalLineCount < LineCount{2U}) {
        try {
            if (readLine().isEmpty()) {
                break;
            }
            ++additionalLineCount;
        } catch (const ConfError &) {
            break;
        }
    }

    const auto minimumLine = requestedLine.retreated(LineCount{2U});
    auto lines = StringList{};
    auto startLine = LineIndex::noIndex();
    for (const auto &[lineIndex, sourceLine] : _recentLines) {
        if (lineIndex < minimumLine || lineIndex > maximumLine) {
            continue;
        }
        if (startLine.isNoIndex()) {
            startLine = lineIndex;
        }
        lines.append(sourceLine.trimmed(lineBreakCharacters, StringSide::Back));
    }
    return lines.isEmpty() ? std::nullopt
                           : std::optional<CodeSnippet>{CodeSnippet{std::move(lines), startLine, "elcl"_el}};
}

void TextStreamSource::close() noexcept {
    _isOpen = false;
    if (_stream) {
        _stream->abort();
        _stream.reset();
    }
}

auto TextStreamSource::readStreamLine() -> std::optional<String> {
    const auto result = _stream->readLine(CpCount{limits::maxLineLength + 1});
    if (result.isTimeout()) {
        throwTimeout();
    }
    if (result.isFinished()) {
        return std::nullopt;
    }
    return result.data();
}

void TextStreamSource::rememberLine(const String &line, const std::optional<ByteIndex> position) {
    if (position.has_value()) {
        rememberLinePosition(_nextLine, *position);
    }
    _recentLines.emplace_back(_nextLine, line);
    _nextLine = _nextLine.advanced(LineCount::one());
    while (_recentLines.size() > 5U) {
        _recentLines.pop_front();
    }
}

auto TextStreamSource::setStreamPosition(const ByteIndex position, const LineIndex nextLine) -> bool {
    if (!_isOpen || _stream == nullptr || !_streamSupportsPositioning) {
        return false;
    }
    if (!_stream->setPosition(position).isSuccess()) {
        return false;
    }
    _recentLines.clear();
    _nextLine = nextLine;
    _atEnd = false;
    return true;
}

void TextStreamSource::sourceIsAtEnd() noexcept {
    _atEnd = true;
    close();
}

void TextStreamSource::throwLineLengthExceeded() {
    close();
    throw ConfError(
        ConfErrorCategory::LimitExceeded,
        StringFormat{"The line exceeds the maximum size of {} bytes."_el}.build(limits::maxLineLength),
        Location{identifier()});
}

void TextStreamSource::throwTimeout() {
    throw ConfError(ConfErrorCategory::IO, "Timed out while reading from the source."_el, Location{identifier()});
}

void TextStreamSource::throwReadError(const ConfErrorCategory category, String title, String description) {
    const auto cause = std::current_exception();
    close();
    throw ConfError{category, std::move(title), std::move(description), Location{identifier()}, cause};
}

}
