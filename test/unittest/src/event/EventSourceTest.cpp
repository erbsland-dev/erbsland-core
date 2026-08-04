// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventSource.hpp>
#include <erbsland/event/impl/CommonEventEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <memory>
#include <stdexcept>

using namespace el::event;

TESTED_TARGETS(EventEditor EventSource CommonEventEditor)
class EventSourceTest final : public el::UnitTest {
    class Editor final : public el::event::impl::CommonEventEditor {
    public:
        Editor(EventSourcePtr source, EventsPtr target, std::function<void()> &callback) :
            CommonEventEditor{std::move(source), std::move(target)}, _callback{callback} {}

    public:
        auto setCallback(std::function<void()> callback) -> Editor & {
            _callback = std::move(callback);
            return *this;
        }

    private:
        std::function<void()> &_callback;
    };

    class Source final : public EventSource {
    public:
        explicit Source(EventsPtr owner) : EventSource{std::move(owner)} {}

    public:
        [[nodiscard]] auto events() -> Editor & override {
            auto target = currentOwnerEvents();
            if (_editor == nullptr) {
                _editor = std::make_unique<Editor>(shared_from_this(), std::move(target), _callback);
            }
            return *_editor;
        }
        void emit() {
            const auto callback = _callback;
            if (callback) {
                callback();
            }
        }

    private:
        std::function<void()> _callback;
        std::unique_ptr<Editor> _editor;
    };

    class DetachedEditor final : public el::event::impl::CommonEventEditor {
    public:
        DetachedEditor(EventSourcePtr source, EventsPtr target) :
            CommonEventEditor{std::move(source), std::move(target)} {}
    };

public:
    void testStableCovariantEditorAndHandlerReplacement() {
        const auto loop = EventLoop::create();
        const auto source = std::make_shared<Source>(loop);
        auto calls = 0;

        loop->invoke([&]() -> void {
            auto &typedEditor = source->events();
            auto &genericEditor = static_cast<EventSource &>(*source).events();
            REQUIRE_EQUAL(&typedEditor, &genericEditor);
            REQUIRE_EQUAL(&typedEditor, &source->events());

            typedEditor.setCallback([&calls]() -> void { calls += 1; });
            source->emit();
            source->events().setCallback([&calls]() -> void { calls += 10; });
            source->emit();
        });

        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(calls, 11);
    }

    void testSourceAndTargetAccess() {
        const auto loop = EventLoop::create();
        const auto source = std::make_shared<Source>(loop);

        loop->invoke([&]() -> void {
            auto &editor = source->events();
            REQUIRE_EQUAL(editor.source(), source);
            REQUIRE_EQUAL(editor.target(), loop);
        });

        REQUIRE(loop->runOnce());
    }

    void testSourceCanBeRetainedThroughEditor() {
        const auto loop = EventLoop::create();
        auto source = std::make_shared<Source>(loop);
        auto weakSource = std::weak_ptr<Source>{source};
        auto retainedSource = EventSourcePtr{};

        loop->invoke([&]() -> void {
            retainedSource = source->events().source();
            source.reset();
            REQUIRE_FALSE(weakSource.expired());
        });

        REQUIRE(loop->runOnce());
        REQUIRE_FALSE(weakSource.expired());
        retainedSource.reset();
        REQUIRE(weakSource.expired());
    }

    void testExpiredCommonEditorSourceIsLogicError() {
        const auto loop = EventLoop::create();
        auto source = std::make_shared<Source>(loop);
        auto editor = std::make_unique<DetachedEditor>(source, loop);
        source.reset();

        REQUIRE_THROWS_AS(el::err::LogicError, editor->source());
        REQUIRE_EQUAL(editor->target(), loop);
    }

    void testCommonEditorRejectsInvalidConstruction() {
        const auto ownerLoop = EventLoop::create();
        const auto otherLoop = EventLoop::create();
        const auto source = std::make_shared<Source>(ownerLoop);

        REQUIRE_THROWS_AS(el::err::ParameterError, DetachedEditor(EventSourcePtr{}, ownerLoop));
        REQUIRE_THROWS_AS(el::err::ParameterError, DetachedEditor(source, EventsPtr{}));
        REQUIRE_THROWS_AS(el::err::LogicError, DetachedEditor(source, otherLoop));
    }

    void testEditorRejectsWrongCurrentLoop() {
        const auto ownerLoop = EventLoop::create();
        const auto otherLoop = EventLoop::create();
        const auto source = std::make_shared<Source>(ownerLoop);
        auto rejected = false;

        otherLoop->invoke([&]() -> void {
            try {
                static_cast<void>(source->events());
            } catch (const el::err::LogicError &) {
                rejected = true;
            }
        });

        REQUIRE(otherLoop->runOnce());
        REQUIRE(rejected);
    }

    void testEditorCallbackExceptionUsesLoopRouting() {
        const auto loop = EventLoop::create();
        const auto source = std::make_shared<Source>(loop);
        loop->invoke([&]() -> void {
            source->events().setCallback([]() -> void { throw std::runtime_error{"editor"}; });
            source->emit();
        });

        REQUIRE(loop->runOnce());
        REQUIRE(loop->hasError());
        REQUIRE(loop->takeError());
    }
};
