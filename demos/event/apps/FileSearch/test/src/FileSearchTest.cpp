// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "../../src/QueryEditor.hpp"
#include "../../src/SearchResults.hpp"
#include "../../src/SearchScanner.hpp"
#include "../../src/SearchView.hpp"

#include <erbsland/BindEvent.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/U32String.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/UnmanagedEventThread.hpp>

#include <latch>

using namespace el::text::literals;
using el::cterm::Key;

TESTED_TARGETS(QueryEditor SearchResults SearchScanner SearchView)
class FileSearchTest final : public el::UnitTest {
    [[nodiscard]] auto fixture() -> el::TempDirectoryPtr {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto root = directory->path();
        (root / "src"_el).operations().createDirectoryOrThrow();
        (root / "src/a.cpp"_el).content().writeTextOrThrow("a"_el);
        (root / "root.cpp"_el).content().writeTextOrThrow("r"_el);
        (root / ".hidden.cpp"_el).content().writeTextOrThrow("h"_el);
        (root / "UPPER.CPP"_el).content().writeTextOrThrow("u"_el);
        return directory;
    }

    [[nodiscard]] auto scan(const el::Path &root, const el::String &pattern) -> el::PathList {
        auto paths = el::PathList{};
        auto completions = 0;
        demo::SearchScanner{root, el::PathGlobPattern::compile(pattern)}.scan(
            []() -> bool { return false; },
            [&](demo::SearchUpdate update) -> void {
                REQUIRE(update.paths.count() <= el::ItemCount{128});
                paths.append(update.paths);
                REQUIRE_FALSE(update.incomplete);
                if (update.finished) {
                    ++completions;
                }
            });
        REQUIRE_EQUAL(completions, 1);
        return paths;
    }

public:
    void testRelativeGlobsAndHiddenEntries() {
        const auto directory = fixture();
        const auto root = directory->path();
        const auto top = scan(root, "*.cpp"_el);
        REQUIRE_EQUAL(top.count(), el::ItemCount{2});
        REQUIRE_EQUAL(top.first(), root / ".hidden.cpp"_el);
        REQUIRE_EQUAL(top.last(), root / "root.cpp"_el);
        const auto recursive = scan(root, "**/*.cpp"_el);
        REQUIRE_EQUAL(recursive.count(), el::ItemCount{3});
        REQUIRE_EQUAL(recursive.last(), root / "src/a.cpp"_el);
        REQUIRE_EQUAL(scan(root, "src"_el).first(), root / "src"_el);
        REQUIRE_EQUAL(scan(root, "**"_el).count(), el::ItemCount{5});
        REQUIRE_THROWS_AS(el::ParseError, el::PathGlobPattern::compile("["_el));
    }

    void testCancellationWithoutMatchesAndIncompleteSearch() {
        const auto directory = fixture();
        auto visited = 0;
        auto delivered = false;
        demo::SearchScanner{directory->path(), el::PathGlobPattern::compile("**/missing"_el)}.scan(
            [&]() -> bool { return ++visited >= 3; }, [&](demo::SearchUpdate) -> void { delivered = true; });
        REQUIRE_FALSE(delivered);
        REQUIRE(visited >= 3);
        auto incomplete = false;
        demo::SearchScanner{directory->path() / "gone"_el, el::PathGlobPattern::compile("**"_el)}.scan(
            []() -> bool { return false; },
            [&](demo::SearchUpdate update) -> void { incomplete = update.finished && update.incomplete; });
        REQUIRE(incomplete);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testProgressiveBatchesAndCancellationAfterFirstBatch() {
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        for (auto i = 0; i < 260; ++i) {
            (directory->path() / el::String::fromInteger(i)).operations().createFileOrThrow();
        }
        auto count = el::ItemCount{};
        auto batches = 0;
        auto finished = false;
        demo::SearchScanner{directory->path(), el::PathGlobPattern::compile("*"_el)}.scan(
            []() -> bool { return false; },
            [&](demo::SearchUpdate update) -> void {
                REQUIRE_FALSE(finished);
                REQUIRE(update.paths.count() <= el::ItemCount{128});
                count += update.paths.count();
                ++batches;
                finished = update.finished;
            });
        REQUIRE(finished);
        REQUIRE(batches >= 3);
        REQUIRE_EQUAL(count, el::ItemCount{260});
        auto cancel = false;
        auto deliveries = 0;
        demo::SearchScanner{directory->path(), el::PathGlobPattern::compile("*"_el)}.scan(
            [&]() -> bool { return cancel; },
            [&](demo::SearchUpdate update) -> void {
                REQUIRE_FALSE(update.finished);
                ++deliveries;
                cancel = true;
            });
        REQUIRE_EQUAL(deliveries, 1);
    }

    void testObsoleteDeliveriesAndSelection() {
        auto results = demo::SearchResults{};
        const auto old = results.generation();
        results.reset();
        results.setStatus("Searching"_el);
        results.receive(old, {el::PathList{el::Path{"obsolete"_el}}, true, true});
        REQUIRE(results.paths().isEmpty());
        REQUIRE_EQUAL(results.status(), "Searching"_el);
        REQUIRE_FALSE(results.selectedPath().has_value());
        auto paths = el::PathList{};
        for (auto i = 0; i < 10; ++i) {
            paths.append(el::Path{el::String::fromInteger(i)});
        }
        results.receive(results.generation(), {paths, false, false});
        REQUIRE_EQUAL(*results.selectedPath(), paths.first());
        results.resize(el::ItemCount{3});
        results.move(true, el::ItemCount{8});
        REQUIRE_EQUAL(results.selected(), el::ItemIndex{8});
        REQUIRE_EQUAL(results.first(), el::ItemIndex{6});
        results.receive(results.generation(), {el::PathList{el::Path{"last"_el}}, true, true});
        REQUIRE_EQUAL(results.selected(), el::ItemIndex{8});
        REQUIRE(results.status().startsWith("Incomplete"_el));
        results.resize(el::ItemCount{1});
        REQUIRE_EQUAL(results.first(), el::ItemIndex{8});
        results.move(false, el::ItemCount{100});
        REQUIRE_EQUAL(results.selected(), el::ItemIndex{});
        results.move(true, el::ItemCount{100});
        REQUIRE_EQUAL(*results.selectedPath(), el::Path{"last"_el});
    }

    void testBoundRepliesRetainTheirOriginalGeneration() {
        const auto directory = fixture();
        const auto main = el::EventLoop::create();
        const auto worker = el::UnmanagedEventThread::create();
        auto results = demo::SearchResults{};
        const auto generation = results.generation();
        auto reply = el::bindEvent(
            main, [&](demo::SearchUpdate update) -> void { results.receive(generation, std::move(update)); });
        worker->loop()->invoke([&]() -> void {
            demo::SearchScanner{directory->path(), el::PathGlobPattern::compile("**"_el)}.scan(
                []() -> bool { return false; }, reply);
            worker->quit();
        });
        worker->start();
        worker->join();
        REQUIRE(main->queuedEventCount() > 0);
        results.reset();
        (void)main->runUntilIdle();
        REQUIRE(results.paths().isEmpty());
        REQUIRE_FALSE(main->hasError());
        REQUIRE_FALSE(worker->workerEventLoop().hasError());
    }

    void testWorkerShutdownCancelsWithoutMainDispatch() {
        const auto directory = fixture();
        const auto worker = el::UnmanagedEventThread::create();
        const auto stopToken = worker->stopToken();
        auto entered = std::latch{1};
        auto release = std::latch{1};
        auto delivered = false;
        worker->loop()->invoke([&]() -> void {
            auto visits = 0;
            demo::SearchScanner{directory->path(), el::PathGlobPattern::compile("**/missing"_el)}.scan(
                [&]() -> bool {
                    if (++visits == 3) {
                        entered.count_down();
                        release.wait();
                    }
                    return stopToken.isStopRequested();
                },
                [&](demo::SearchUpdate) -> void { delivered = true; });
        });
        worker->start();
        entered.wait();
        worker->quit();
        release.count_down();
        worker->join();
        REQUIRE_FALSE(delivered);
    }

    void testUnicodeEditingAndNavigation() {
        auto editor = demo::QueryEditor{};
        REQUIRE(editor.edit(Key{U'界'}));
        REQUIRE(editor.edit(Key{Key::Combined, el::text::U32String{U"e\u0301"_el}}));
        REQUIRE_EQUAL(editor.cursor(), el::CpIndex{3});
        REQUIRE_FALSE(editor.edit(Key::Left));
        REQUIRE_EQUAL(editor.cursor(), el::CpIndex{1});
        REQUIRE(editor.edit(Key::Delete));
        REQUIRE_EQUAL(editor.text(), "界"_el);
        REQUIRE(editor.edit(Key::Space));
        REQUIRE_FALSE(editor.edit(Key::Home));
        REQUIRE(editor.edit(Key{U'*'}));
        REQUIRE_EQUAL(editor.text(), "*界 "_el);
        REQUIRE_FALSE(editor.edit(Key::End));
        REQUIRE(editor.edit(Key::Backspace));
        REQUIRE_EQUAL(editor.text(), "*界"_el);
        REQUIRE_FALSE(editor.edit(Key{U'\n'}));
        REQUIRE_FALSE(editor.edit(Key::Up));
    }

    void testRenderingCroppingCursorAndResize() {
        auto view = demo::SearchView{};
        auto query = demo::QueryEditor{};
        auto results = demo::SearchResults{};
        const auto root = el::Path::systemTempDirectoryOrThrow();
        const auto path = root / "a-very-long-directory-name-to-crop/界.cpp"_el;
        results.receive(results.generation(), {el::PathList{path}, true, false});
        const auto cropped = demo::SearchView::cropLeft(path.toNativeString(), el::block::Coordinate{15});
        REQUIRE(cropped.displayWidth() <= 15);
        REQUIRE(cropped.at(el::cterm::BlockIndex{}) == U'…');
        const auto &frame = view.render(root, query, results, {60, 15}, true);
        REQUIRE(frame.toString().contains("界.cpp"_el));
        REQUIRE_EQUAL(frame.get({1, 4}).color().bg(), el::cterm::bg::Blue);
        REQUIRE_EQUAL(frame.get({2, 12}).color().bg(), el::cterm::bg::BrightWhite);
        const auto &off = view.render(root, query, results, {60, 15}, false);
        REQUIRE_EQUAL(off.get({2, 12}).color().bg(), el::cterm::bg::Black);
        REQUIRE(view.render(root, query, results, {40, 8}, true).toString().contains("Resize"_el));
        for (auto i = 0; i < 80; ++i) {
            REQUIRE(query.edit(Key{U'界'}));
        }
        REQUIRE_EQUAL(view.render(root, query, results, {60, 15}, true).size(), (el::block::Size{60, 15}));
        REQUIRE_EQUAL(results.rows(), el::ItemCount{5});
    }
};
