// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../stream/TextInputStream.hpp"
#include "../../Input.hpp"

#include <optional>

namespace erbsland::re::impl {

/// An input that reads and captures text from a seekable text stream.
/// @tested{RegExStreamInputTest}
class StreamInput final : public Input {
public:
    /// Create an input for a seekable text stream.
    [[nodiscard]] static auto create(const stream::TextInputStreamPtr &stream) -> InputPtr;

    explicit StreamInput(stream::TextInputStreamPtr stream);

public: // implement Input
    [[nodiscard]] auto read() -> CharAndPosition override;
    [[nodiscard]] auto peek() -> CharAndPosition override;
    void skip(unit::CpLength characterCount) override;
    [[nodiscard]] auto createMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList) -> MatchPtr override;

private:
    [[nodiscard]] auto readFromStream() -> CharAndPosition;
    [[nodiscard]] auto readCapture(const CaptureRange &range) -> text::String;
    void setPosition(unit::ByteIndex position);

private:
    stream::TextInputStreamPtr _stream;              ///< The seekable source stream.
    std::optional<CharAndPosition> _peekedCharacter; ///< A character read for lookahead.
};

}
