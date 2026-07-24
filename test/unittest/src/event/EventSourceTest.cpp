// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/event/EventEditor.hpp>
#include <erbsland/event/EventLoop.hpp>
#include <erbsland/event/EventSource.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <functional>
#include <memory>
#include <stdexcept>

using namespace el::event;

TESTED_TARGETS(EventEditor EventSource)
class EventSourceTest final : public el::UnitTest {
    class Editor final : public EventEditor {
    public:
        explicit Editor(EventsPtr target) : EventEditor{std::move(target)} {}
        std::function<void()> callback;
    };
    using EditorPtr = std::shared_ptr<Editor>;

    class Source final : public EventSource, public std::enable_shared_from_this<Source> {
    public:
        explicit Source(EventsPtr owner) : EventSource{std::move(owner)} {}
        [[nodiscard]] auto events() -> EditorPtr {
            auto result = std::make_shared<Editor>(currentOwnerEvents());
            registerEditor(result);
            return result;
        }
        void emit() {
            for (const auto &editor : connectedEditors<Editor>()) {
                if (editor->callback) {
                    editor->callback();
                }
            }
        }
        [[nodiscard]] auto connectedCount() const -> std::size_t { return connectedEditors<Editor>().size(); }
    };

public:
    void testEditorLifetimeControlsSubscription() {
        const auto loop = EventLoop::create();
        const auto source = std::make_shared<Source>(loop);
        auto firstCount = 0;
        auto secondCount = 0;
        auto first = EditorPtr{};
        auto second = EditorPtr{};
        auto *firstCountPtr = &firstCount;
        auto *secondCountPtr = &secondCount;

        loop->invoke([&]() -> void {
            first = source->events();
            second = source->events();
            first->callback = [firstCountPtr]() -> void { *firstCountPtr += 1; };
            second->callback = [secondCountPtr]() -> void { *secondCountPtr += 1; };
            REQUIRE_EQUAL(source->connectedCount(), std::size_t{2U});
            source->emit();
            first.reset();
            source->emit();
            second->disconnect();
            source->emit();
        });

        REQUIRE(loop->runOnce());
        REQUIRE_EQUAL(firstCount, 1);
        REQUIRE_EQUAL(secondCount, 2);
    }

    void testSourceDestructionDisconnectsEditors() {
        const auto loop = EventLoop::create();
        auto source = std::make_shared<Source>(loop);
        auto editor = EditorPtr{};

        loop->invoke([&]() -> void {
            editor = source->events();
            REQUIRE(editor->isConnected());
            source.reset();
        });

        REQUIRE(loop->runOnce());
        REQUIRE_FALSE(editor->isConnected());
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
        auto editor = EditorPtr{};
        loop->invoke([&]() -> void {
            editor = source->events();
            editor->callback = []() -> void { throw std::runtime_error{"editor"}; };
            source->emit();
        });

        REQUIRE(loop->runOnce());
        REQUIRE(loop->hasError());
        REQUIRE(loop->takeError() != nullptr);
    }
};
