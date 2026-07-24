// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/ParameterError.hpp>
#include <erbsland/re/Match.hpp>
#include <erbsland/re/RegEx.hpp>
#include <erbsland/re/StdFormat.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/stream/TextInputStream.hpp>
#include <erbsland/text/EncodingError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <memory>
#include <utility>

using namespace el::text::literals;
using namespace el::re;

TESTED_TARGETS(RegEx)
TAGS(Api Stream)
class RegExStreamInputTest final : public el::UnitTest {
private:
    class MemoryTextInputStream final : public el::stream::TextInputStream {
    public:
        explicit MemoryTextInputStream(
            el::text::String text, const bool supportsPositioning = true, const bool timeout = false) :
            _text{std::move(text)}, _supportsPositioning{supportsPositioning}, _timeout{timeout} {}

    public: // implement StreamPositioning
        [[nodiscard]] auto supportsPositioning() const noexcept -> bool override { return _supportsPositioning; }
        [[nodiscard]] auto position() const -> el::unit::ByteIndex override {
            if (!_supportsPositioning) {
                return StreamPositioning::position();
            }
            return _position;
        }
        auto setPosition(const el::unit::ByteIndex position) -> el::stream::StreamPositionStatus override {
            if (!_supportsPositioning) {
                return StreamPositioning::setPosition(position);
            }
            _position = position;
            return el::stream::StreamPositionStatus::Success;
        }

    public: // implement TextInputStream
        [[nodiscard]] auto encoding() const noexcept -> el::text::StringEncoding override {
            return el::text::StringEncoding::Utf8;
        }
        [[nodiscard]] auto effectiveEncoding() const noexcept -> el::text::StringEncoding override {
            return el::text::StringEncoding::Utf8;
        }
        [[nodiscard]] auto inputSettings() const noexcept -> const el::stream::InputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return !_timeout; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return _timeout ? el::stream::StreamWaitStatus::Timeout : el::stream::StreamWaitStatus::Ready;
        }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }
        [[nodiscard]] auto readChar() -> el::stream::StreamReadResult<el::text::Char> override {
            if (_timeout) {
                return {el::stream::StreamReadStatus::Timeout, {}};
            }
            if (_position >= el::unit::ByteIndex::end(_text.length())) {
                return {el::stream::StreamReadStatus::Finished, {}};
            }
            _readCount += 1U;
            return {el::stream::StreamReadStatus::Data, _text.readCharAndAdvance(_position)};
        }
        [[nodiscard]] auto read(const el::unit::CpLength maximum)
            -> el::stream::StreamReadResult<el::text::String> override {
            auto result = el::text::StringEditor{};
            while (result.characterLength() < maximum) {
                const auto character = readChar();
                if (character.isTimeout()) {
                    return {el::stream::StreamReadStatus::Timeout, {}};
                }
                if (character.isFinished()) {
                    return result.isEmpty()
                        ? el::stream::StreamReadResult<el::text::String>{el::stream::StreamReadStatus::Finished, {}}
                        : el::stream::StreamReadResult<el::text::String>{
                              el::stream::StreamReadStatus::Data, std::move(result)};
                }
                result.append(character.data());
            }
            return {el::stream::StreamReadStatus::Data, std::move(result)};
        }
        [[nodiscard]] auto readLine(const el::unit::CpLength maximum)
            -> el::stream::StreamReadResult<el::text::String> override {
            return read(maximum);
        }
        [[nodiscard]] auto readAll(const el::unit::CpLength maximum)
            -> el::stream::StreamReadResult<el::text::String> override {
            return read(maximum);
        }

    public: // diagnostics
        [[nodiscard]] auto readCount() const noexcept -> std::size_t { return _readCount; }

    private:
        el::text::String _text;
        el::unit::ByteIndex _position;
        bool _supportsPositioning;
        bool _timeout;
        std::size_t _readCount{0U};
        el::stream::InputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

    [[nodiscard]] static auto createStream(
        el::text::String text, const bool supportsPositioning = true, const bool timeout = false)
        -> el::stream::TextInputStreamPtr {
        return std::make_shared<MemoryTextInputStream>(std::move(text), supportsPositioning, timeout);
    }

public:
    void testMatchingOperationsAndCapturedContent() {
        const auto expression = RegEx::compile("(a)(b)"_el);

        const auto match = expression->match(createStream("ab!"_el));
        REQUIRE(match != nullptr);
        REQUIRE_EQUAL(match->content(), "ab"_el);
        REQUIRE_EQUAL(match->content(1), "a"_el);
        REQUIRE_EQUAL(match->content(2), "b"_el);

        const auto fullMatch = expression->fullMatch(createStream("ab"_el));
        REQUIRE(fullMatch != nullptr);
        REQUIRE_EQUAL(fullMatch->content(), "ab"_el);

        const auto first = expression->findFirst(createStream("!ab!"_el));
        REQUIRE(first != nullptr);
        REQUIRE_EQUAL(first->begin(), 1U);
        REQUIRE_EQUAL(first->end(), 3U);

        const auto all = expression->collectAll(createStream("ab!ab"_el));
        REQUIRE_EQUAL(all.size(), 2U);
        REQUIRE_EQUAL(all[0]->content(), "ab"_el);
        REQUIRE_EQUAL(all[1]->content(2), "b"_el);
    }

    void testFindAllPreservesStreamCursorForEnumeration() {
        const auto expression = RegEx::compile("a"_el);
        const auto stream = createStream("a-a-a"_el);
        auto count = 0U;
        for (const auto &match : expression->findAll(stream)) {
            REQUIRE(match != nullptr);
            REQUIRE_EQUAL(match->content(), "a"_el);
            count += 1U;
        }
        REQUIRE_EQUAL(count, 3U);
    }

    void testUnicodeCaptureUsesBytePositions() {
        const auto expression = RegEx::compile("(é)"_el);
        const auto match = expression->findFirst(createStream("xé!"_el));

        REQUIRE(match != nullptr);
        REQUIRE_EQUAL(match->begin(), 1U);
        REQUIRE_EQUAL(match->end(), 3U);
        REQUIRE_EQUAL(match->content(), "é"_el);
    }

    void testCapturedContentSurvivesStreamReuse() {
        const auto expression = RegEx::compile("(a)(b)"_el);
        const auto stream = createStream("ab"_el);
        const auto match = expression->fullMatch(stream);
        REQUIRE(match != nullptr);

        REQUIRE_EQUAL(stream->setPosition({}), el::stream::StreamPositionStatus::Success);
        REQUIRE_EQUAL(stream->readChar().data(), el::text::Char{U'a'});
        REQUIRE_EQUAL(match->content(), "ab"_el);
        REQUIRE_EQUAL(match->content(2), "b"_el);
    }

    void testInvalidAndTimeoutStreamsFailExplicitly() {
        const auto expression = RegEx::compile("a"_el);
        const auto nullStream = el::stream::TextInputStreamPtr{};
        REQUIRE_THROWS_AS(el::err::ParameterError, expression->match(nullStream));
        REQUIRE_THROWS_AS(el::err::ParameterError, expression->match(createStream("a"_el, false)));
        REQUIRE_THROWS_AS(el::stream::StreamError, expression->match(createStream("a"_el, true, true)));
    }

    void testNullInputPolicyIsAppliedDuringReading() {
        auto patternSettings = Settings{};
        patternSettings.enableFeature(Feature::AcceptNullInPattern);
        const auto expression = RegEx::compile("\\x00"_el, {}, patternSettings);
        auto text = el::text::StringEditor{};
        text.append(el::text::Char{U'\0'});

        REQUIRE(expression->fullMatch(createStream(text)) != nullptr);

        auto strictSettings = patternSettings;
        strictSettings.disableFeature(Feature::AcceptNullInInput);
        const auto strictExpression = RegEx::compile("\\x00"_el, {}, strictSettings);
        const auto stream = std::make_shared<MemoryTextInputStream>(text);
        REQUIRE_THROWS_AS(el::text::EncodingError, strictExpression->fullMatch(stream));
        REQUIRE_EQUAL(stream->readCount(), 1U);
    }
};
