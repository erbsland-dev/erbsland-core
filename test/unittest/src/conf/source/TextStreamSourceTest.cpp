// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ConfTestHelper.hpp"

#include <erbsland/conf/impl/source/TextStreamSource.hpp>
#include <erbsland/conf/StdFormat.hpp>
#include <erbsland/path/PathError.hpp>
#include <erbsland/stream/StreamError.hpp>
#include <erbsland/stream/TextInputStream.hpp>

#include <deque>
#include <stdexcept>

using namespace el::conf;
using namespace el::text::literals;

TESTED_TARGETS(TextStreamSource)
class TextStreamSourceTest final : public UNITTEST_SUBCLASS(ConfTestHelper) {
    class TestTextInputStream final : public el::stream::TextInputStream {
    public:
        enum class Action { Data, Timeout, Finished, Failure };

        struct Step {
            Action action;
            el::text::String text;
        };

        explicit TestTextInputStream(std::deque<Step> steps) : _steps{std::move(steps)} {}

    public:
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
        [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }

        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return el::stream::StreamWaitStatus::Ready;
        }

        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }

        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

        [[nodiscard]] auto readChar() -> el::stream::StreamReadResult<el::text::Char> override {
            return {el::stream::StreamReadStatus::Finished, el::text::Char{}};
        }

        [[nodiscard]] auto read(el::unit::CpLength maximum) -> el::stream::StreamReadResult<el::text::String> override {
            return readLine(maximum);
        }

        [[nodiscard]] auto readLine(el::unit::CpLength) -> el::stream::StreamReadResult<el::text::String> override {
            if (_steps.empty()) {
                return {el::stream::StreamReadStatus::Finished, el::text::String{}};
            }
            auto step = std::move(_steps.front());
            _steps.pop_front();
            switch (step.action) {
            case Action::Data:
                return {el::stream::StreamReadStatus::Data, std::move(step.text)};
            case Action::Timeout:
                return {el::stream::StreamReadStatus::Timeout, {}};
            case Action::Finished:
                return {el::stream::StreamReadStatus::Finished, {}};
            case Action::Failure:
                throw el::stream::StreamError{el::stream::StreamErrorContext{
                    "Reading the Simulated Stream Failed"_el, "The simulated stream failed."_el}};
            }
            throw std::logic_error{"unexpected test stream action"};
        }

        [[nodiscard]] auto readAll(el::unit::CpLength maximum)
            -> el::stream::StreamReadResult<el::text::String> override {
            return readLine(maximum);
        }

    private:
        std::deque<Step> _steps;
        el::stream::InputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

    class TestSource final : public impl::TextStreamSource {
    public:
        explicit TestSource(el::stream::TextInputStreamPtr stream) : _stream{std::move(stream)} {}

        [[nodiscard]] auto identifier() const noexcept -> SourceIdentifierPtr override { return _identifier; }

    protected:
        [[nodiscard]] auto createStream() -> el::stream::TextInputStreamPtr override { return _stream; }

    private:
        el::stream::TextInputStreamPtr _stream;
        SourceIdentifierPtr _identifier{SourceIdentifier::createForText()};
    };

    class FailingOpenSource final : public impl::TextStreamSource {
    public:
        [[nodiscard]] auto identifier() const noexcept -> SourceIdentifierPtr override { return _identifier; }

    protected:
        [[nodiscard]] auto createStream() -> el::stream::TextInputStreamPtr override {
            throw el::path::PathError{el::path::PathErrorContext{"Opening the Simulated Path Failed"_el}};
        }

    private:
        SourceIdentifierPtr _identifier{SourceIdentifier::createForText()};
    };

public:
    void testOpenPreservesRealPathErrorCause() {
        auto source = std::make_shared<FailingOpenSource>();
        try {
            source->open();
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::IO);
            REQUIRE(error.cause());
            REQUIRE_THROWS_AS(el::path::PathError, std::rethrow_exception(error.cause()));
        }
        REQUIRE_FALSE(source->isOpen());
    }

    void testTimeoutIsTranslatedAndReadingCanContinue() {
        using Action = TestTextInputStream::Action;
        using Step = TestTextInputStream::Step;
        auto stream = std::make_shared<TestTextInputStream>(
            std::deque<Step>{{Action::Timeout, {}}, {Action::Data, "line\n"_el}, {Action::Finished, {}}});
        auto source = std::make_shared<TestSource>(stream);
        source->open();

        try {
            static_cast<void>(source->readLine());
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::IO);
        }
        REQUIRE(source->isOpen());
        REQUIRE_EQUAL(source->readLine(), "line\n"_el);
        REQUIRE_FALSE(source->atEnd());
        REQUIRE(source->readLine().isEmpty());
        REQUIRE(source->atEnd());
    }

    void testTimeoutAndStreamErrorAreTranslated() {
        using Action = TestTextInputStream::Action;
        using Step = TestTextInputStream::Step;
        auto timeoutStream = std::make_shared<TestTextInputStream>(
            std::deque<Step>{{Action::Data, "first\n"_el}, {Action::Timeout, {}}, {Action::Finished, {}}});
        auto timeoutSource = std::make_shared<TestSource>(timeoutStream);
        timeoutSource->open();
        REQUIRE_EQUAL(timeoutSource->readLine(), "first\n"_el);
        REQUIRE_THROWS_AS(ConfError, timeoutSource->readLine());

        auto errorStream = std::make_shared<TestTextInputStream>(std::deque<Step>{{Action::Failure, {}}});
        auto errorSource = std::make_shared<TestSource>(errorStream);
        errorSource->open();
        try {
            static_cast<void>(errorSource->readLine());
            REQUIRE(false);
        } catch (const ConfError &error) {
            REQUIRE_EQUAL(error.category(), ConfErrorCategory::IO);
            REQUIRE(error.cause());
            try {
                std::rethrow_exception(error.cause());
                REQUIRE(false);
            } catch (const el::stream::StreamError &streamError) {
                REQUIRE_EQUAL(streamError.reason(), "Reading the Simulated Stream Failed"_el);
            }
        }
        REQUIRE_FALSE(errorSource->isOpen());
    }

    void testCodeSnippetReadsOnlyNeededContextAndSurvivesClose() {
        using Action = TestTextInputStream::Action;
        using Step = TestTextInputStream::Step;
        auto stream = std::make_shared<TestTextInputStream>(std::deque<Step>{
            {Action::Data, "zero\r\n"_el},
            {Action::Data, "one\n"_el},
            {Action::Data, "two\n"_el},
            {Action::Data, "three\n"_el},
            {Action::Data, "four\n"_el},
            {Action::Data, "five"_el},
            {Action::Finished, {}}});
        auto source = std::make_shared<TestSource>(stream);
        source->open();

        REQUIRE_EQUAL(source->readLine(), "zero\r\n"_el);
        const auto early = source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex::zero()});
        REQUIRE(early.has_value());
        REQUIRE_EQUAL(early->lines.count(), el::unit::ElementCount{3U});
        REQUIRE_EQUAL(early->lines.get(el::unit::ElementIndex::zero()), "zero"_el);
        REQUIRE_EQUAL(early->lines.get(el::unit::ElementIndex::one()), "one"_el);
        REQUIRE_EQUAL(early->lines.get(el::unit::ElementIndex{2U}), "two"_el);

        REQUIRE_EQUAL(source->readLine(), "three\n"_el);
        REQUIRE_EQUAL(source->readLine(), "four\n"_el);
        const auto middle = source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{3U}});
        REQUIRE(middle.has_value());
        REQUIRE_EQUAL(middle->startLine, el::unit::LineIndex{1U});
        REQUIRE_EQUAL(middle->lines.count(), el::unit::ElementCount{5U});
        REQUIRE_EQUAL(middle->lines.get(el::unit::ElementIndex{4U}), "five"_el);
        REQUIRE_FALSE(source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex::zero()}).has_value());

        source->close();
        REQUIRE(source->codeSnippet(el::unit::CodeLocation{el::unit::LineIndex{3U}}).has_value());
        REQUIRE_FALSE(source->codeSnippet(el::unit::CodeLocation{}).has_value());
    }
};
