// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../../src/SourceReadMethod.hpp"
#include "../../src/SourceTreeWorkerWorkload.hpp"

#include <erbsland/ItemCount.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/profiling/WorkerExecutionContext.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <stop_token>

using namespace el::text::literals;

TESTED_TARGETS(SourceTreeWorkerWorkload)
class SourceTreeWorkerWorkloadTest final : public el::UnitTest {
public:
    void testCancelledBatchDoesNotWalkOrCountPasses() {
        auto stopSource = std::stop_source{};
        stopSource.request_stop();
        for (const auto method : {app::path::SourceReadMethod::Text, app::path::SourceReadMethod::Data}) {
            auto worker = app::path::SourceTreeWorkerWorkload{method, el::Path{}};
            const auto result = worker.execute({.operations = 3U, .stopToken = stopSource.get_token()});
            REQUIRE_EQUAL(result.operations, 0U);
            REQUIRE(result.metrics.count() == el::ItemCount{3U});
            for (const auto counter : result.metrics) {
                REQUIRE_EQUAL(counter, 0U);
            }
            REQUIRE_EQUAL(result.sink, 0U);
        }
    }
};
