// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "../../src/ConfigurationBatch.hpp"

#include <erbsland/conf/ConfError.hpp>
#include <erbsland/DiagnosticHelper.hpp>
#include <erbsland/EventLoop.hpp>
#include <erbsland/EventLoopErrorPolicy.hpp>
#include <erbsland/EventLoopOptions.hpp>
#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/PlainTextRenderer.hpp>
#include <erbsland/String.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/TextDocument.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/UnmanagedEventThread.hpp>

#include <exception>
#include <memory>

using namespace el::text::literals;

TESTED_TARGETS(ConfigurationBatch)
class ConfigurationBatchTest final : public el::UnitTest {
public:
    void setUp() override {
        _directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        path("good.elcl"_el).content().writeTextOrThrow("[Server]\nPort: 8080\n"_el);
        path("other.elcl"_el).content().writeTextOrThrow("[Worker]\nName: \"Index builder\"\n"_el);
        path("broken.elcl"_el).content().writeTextOrThrow("[Server]\nPort: ???\n"_el);
        path("damaged.elcl"_el).content().writeTextOrThrow("[Worker\nName: \"Queue\"\n"_el);
    }

    void tearDown() override {
        _worker.reset();
        _batch.reset();
        _directory.reset();
    }

    void testStoreChecksLaterFilesAndRetainsFailuresInOrder() {
        run(el::List<el::Path>{
            path("broken.elcl"_el), path("good.elcl"_el), path("missing.elcl"_el), path("other.elcl"_el)});
        REQUIRE_EQUAL(_batch->checkedCount(), el::ItemCount{4});
        REQUIRE_EQUAL(_batch->passedPaths().count(), el::ItemCount{2});
        REQUIRE_EQUAL(_batch->passedPaths().first(), path("good.elcl"_el));
        REQUIRE_EQUAL(_batch->passedPaths().last(), path("other.elcl"_el));
        REQUIRE(_worker->workerEventLoop().isQuitRequested());
        REQUIRE(takeDiagnostic().contains("broken.elcl"_el));
        REQUIRE(takeDiagnostic().contains("missing.elcl"_el));
        REQUIRE_FALSE(_worker->workerEventLoop().hasError());
    }

    void testLimitOneStopsBeforeTheNextFileAndKeepsTheError() {
        run(el::List<el::Path>{path("broken.elcl"_el), path("good.elcl"_el)}, el::ItemCount{1});
        REQUIRE_EQUAL(_batch->checkedCount(), el::ItemCount{1});
        REQUIRE(_batch->passedPaths().isEmpty());
        // Stop returns from dispatch before either the next check or the final quit callback.
        REQUIRE_FALSE(_worker->workerEventLoop().isQuitRequested());
        REQUIRE(takeDiagnostic().contains("broken.elcl"_el));
        REQUIRE_FALSE(_worker->workerEventLoop().hasError());
    }

    void testLimitTwoContinuesAfterFirstFailureAndStopsAtSecond() {
        run(
            el::List<el::Path>{
                path("broken.elcl"_el), path("good.elcl"_el), path("damaged.elcl"_el), path("other.elcl"_el)},
            el::ItemCount{2});
        REQUIRE_EQUAL(_batch->checkedCount(), el::ItemCount{3});
        REQUIRE_EQUAL(_batch->passedPaths().count(), el::ItemCount{1});
        REQUIRE_FALSE(_worker->workerEventLoop().isQuitRequested());
        REQUIRE(takeDiagnostic().contains("broken.elcl"_el));
        REQUIRE(takeDiagnostic().contains("damaged.elcl"_el));
        REQUIRE_FALSE(_worker->workerEventLoop().hasError());
    }

    void testBelowLimitReachesNormalCompletion() {
        run(el::List<el::Path>{path("broken.elcl"_el), path("good.elcl"_el)}, el::ItemCount{2});
        REQUIRE_EQUAL(_batch->checkedCount(), el::ItemCount{2});
        REQUIRE_EQUAL(_batch->passedPaths().count(), el::ItemCount{1});
        REQUIRE(_worker->workerEventLoop().isQuitRequested());
        REQUIRE(takeDiagnostic().contains("broken.elcl"_el));
        REQUIRE_FALSE(_worker->workerEventLoop().hasError());
    }

    void testSuccessfulBatchHasNoErrors() {
        run(el::List<el::Path>{path("good.elcl"_el), path("other.elcl"_el)}, el::ItemCount{1});
        REQUIRE_EQUAL(_batch->checkedCount(), el::ItemCount{2});
        REQUIRE_EQUAL(_batch->passedPaths().count(), el::ItemCount{2});
        REQUIRE(_worker->workerEventLoop().isQuitRequested());
        REQUIRE_FALSE(_worker->workerEventLoop().hasError());
    }

    void testIncludesAreLoadedAndMissingIncludesRemainDiagnosable() {
        path("includes.elcl"_el).content().writeTextOrThrow("@include: \"good.elcl\"\n"_el);
        path("missing-include.elcl"_el).content().writeTextOrThrow("@include: \"missing.elcl\"\n"_el);
        run(el::List<el::Path>{path("includes.elcl"_el), path("missing-include.elcl"_el), path("other.elcl"_el)});
        REQUIRE_EQUAL(_batch->checkedCount(), el::ItemCount{3});
        REQUIRE_EQUAL(_batch->passedPaths().count(), el::ItemCount{2});
        const auto diagnostic = takeDiagnostic();
        REQUIRE(diagnostic.contains("missing.elcl"_el));
        REQUIRE_FALSE(_worker->workerEventLoop().hasError());
    }

    void testLimitOnFinalFileStillReportsThatFile() {
        run(el::List<el::Path>{path("good.elcl"_el), path("broken.elcl"_el)}, el::ItemCount{1});
        REQUIRE_EQUAL(_batch->checkedCount(), el::ItemCount{2});
        REQUIRE_EQUAL(_batch->passedPaths().count(), el::ItemCount{1});
        REQUIRE(takeDiagnostic().contains("broken.elcl"_el));
        REQUIRE_FALSE(_worker->workerEventLoop().hasError());
    }

private:
    auto path(const el::String &name) const -> el::Path { return _directory->path() / name; }

    void run(const el::List<el::Path> &paths, const el::ItemCount maximumErrors = {}) {
        _batch = std::make_unique<demo::ConfigurationBatch>(paths, maximumErrors);
        _worker =
            el::UnmanagedEventThread::create(el::EventLoopOptions{}.setErrorPolicy(el::EventLoopErrorPolicy::Store));
        _batch->schedule(_worker->workerEventLoop());
        _worker->start();
        _worker->join();
    }

    auto takeDiagnostic() -> el::String {
        const auto error = _worker->workerEventLoop().takeError();
        REQUIRE(error != nullptr);
        REQUIRE_THROWS_AS(el::conf::ConfError, std::rethrow_exception(error));
        const auto document = el::DiagnosticHelper::documentFromError(error);
        return el::PlainTextRenderer{document}.build();
    }

private:
    el::TempDirectoryPtr _directory;
    std::unique_ptr<demo::ConfigurationBatch> _batch;
    el::UnmanagedEventThreadPtr _worker;
};
