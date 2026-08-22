// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MockStringMatch.hpp"

#include <erbsland/re/CharAndPosition.hpp>
#include <erbsland/re/Input.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/text/impl/UnsafeU8StringAccess.hpp>
#include <erbsland/text/u8/impl/U8Encoding.hpp>

using namespace erbsland::re;
namespace text = erbsland::text;
namespace unit = erbsland::unit;

/// Test input backed by a string that records read operations.
/// @notest{Used only by regular-expression engine unit tests.}
class MockStringInput : public Input {
public:
    /// Create an input that retains a copy of `text`.
    explicit MockStringInput(const text::String &text) noexcept : _text{text} {}

public:
    [[nodiscard]] auto read() -> CharAndPosition override {
        callLog.emplace_back("read()");
        const auto startPosition = position;
        return {readCharacter(position), startPosition.toSizeT()};
    }

    [[nodiscard]] auto peek() -> CharAndPosition override {
        callLog.emplace_back("peek()");
        auto readPosition = position;
        return {readCharacter(readPosition), position.toSizeT()};
    }

    void skip(const unit::CpLength characterCount) override {
        callLog.emplace_back(std::format("skip({})", characterCount.toSizeT()));
        for (auto count = unit::CpLength{}; count < characterCount; ++count) {
            if (readCharacter(position).isEndOfData()) {
                break;
            }
        }
    }

    [[nodiscard]] auto createMatch(CaptureGroupList captureGroupList) -> MatchPtr override {
        callLog.emplace_back("createMatch()");
        return std::make_shared<MockStringMatch>(std::move(captureGroupList), _text);
    }

private:
    /// Decode one character and advance the supplied byte position.
    [[nodiscard]] auto readCharacter(unit::ByteIndex &readPosition) const -> text::Char {
        const auto data = text::impl::UnsafeU8StringAccess{_text}.dataSpan();
        if (readPosition.toSizeT() >= data.size()) {
            return text::Char::endOfData();
        }
        return text::impl::utf8::decodeCharOrThrow(data, readPosition);
    }

public:
    text::String _text;
    unit::ByteIndex position;
    mutable std::vector<std::string> callLog;
};

using MockStringInputPtr = std::shared_ptr<MockStringInput>;
