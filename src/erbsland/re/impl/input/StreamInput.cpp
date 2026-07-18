// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamInput.hpp"

#include "StreamMatch.hpp"

#include "../error/InternalError.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../stream/StreamReadStatus.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../CaptureRange.hpp"

#include <vector>

namespace erbsland::re::impl {

using namespace text::literals;

auto StreamInput::create(const stream::TextInputStreamPtr &stream) -> InputPtr {
    return std::make_shared<StreamInput>(stream);
}

StreamInput::StreamInput(stream::TextInputStreamPtr stream) : _stream{std::move(stream)} {
    if (_stream == nullptr) {
        throw err::ParameterError{"Input stream cannot be null."_el, "input"_el};
    }
    if (!_stream->supportsPositioning()) {
        throw err::ParameterError{
            "Regular expression matching requires a text stream with positioning support."_el, "input"_el};
    }
}

auto StreamInput::read() -> CharAndPosition {
    if (_peekedCharacter.has_value()) {
        const auto result = _peekedCharacter.value();
        _peekedCharacter.reset();
        return result;
    }
    return readFromStream();
}

auto StreamInput::peek() -> CharAndPosition {
    if (!_peekedCharacter.has_value()) {
        _peekedCharacter = readFromStream();
    }
    return _peekedCharacter.value();
}

void StreamInput::skip(const unit::CpLength characterCount) {
    for (auto index = unit::CpLength{}; index < characterCount; ++index) {
        if (read().character.isEndOfData()) {
            return;
        }
    }
}

auto StreamInput::createMatch(CaptureGroupList captureGroupList) -> MatchPtr {
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        !captureGroupList.empty(), "Capture group list must contain at least one element (the whole string)"_el);
    const auto restorePosition = _stream->position();
    auto content = std::vector<text::String>{};
    content.reserve(captureGroupList.size());
    try {
        for (const auto &group : captureGroupList) {
            content.emplace_back(readCapture(group.range()));
        }
    } catch (...) {
        setPosition(restorePosition);
        throw;
    }
    setPosition(restorePosition);
    return std::make_shared<StreamMatch>(std::move(captureGroupList), std::move(content));
}

auto StreamInput::readFromStream() -> CharAndPosition {
    const auto position = _stream->position();
    const auto result = _stream->readChar();
    if (result.isTimeout()) {
        _stream->throwError(
            "Failed to read text for regular expression matching."_el,
            "The text stream timed out before it could provide the next character."_el);
    }
    if (result.isFinished()) {
        return {text::Char::endOfData(), position.toSizeT()};
    }
    return {result.data(), position.toSizeT()};
}

auto StreamInput::readCapture(const CaptureRange &range) -> text::String {
    const auto begin = unit::ByteIndex::fromSizeT(range.begin());
    const auto end = unit::ByteIndex::fromSizeT(range.end());
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(begin <= end, "Capture group begin index must not exceed end index"_el);
    setPosition(begin);
    auto content = text::StringEditor{};
    while (_stream->position() < end) {
        const auto result = _stream->readChar();
        if (result.isTimeout()) {
            _stream->throwError(
                "Failed to capture regular expression match content."_el,
                "The text stream timed out while reading captured text."_el);
        }
        if (result.isFinished()) {
            _stream->throwError(
                "Failed to capture regular expression match content."_el,
                "The text stream ended before the complete captured range could be read."_el);
        }
        content.append(result.data());
    }
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        _stream->position() == end, "Text stream position must end at the capture range end"_el);
    return content;
}

void StreamInput::setPosition(const unit::ByteIndex position) {
    if (_stream->setPosition(position).isTimeout()) {
        _stream->throwError(
            "Failed to position text for regular expression matching."_el,
            "The text stream timed out before its position could be changed."_el);
    }
}

}
