// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/EnvironmentVariables.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Subprocess.hpp>
#include <erbsland/unittest/FileHelper.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/WorkerPool.hpp>

#include <cstdlib>
#include <exception>
#include <latch>
#include <new>
#include <stdexcept>

using namespace el::text::literals;

TESTED_TARGETS(WorkerPool)
class WorkerPoolFatalTest final : public el::UnitTest {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFatalBoundaries() {
        for (const auto mode : {"memory"_el, "logic"_el, "standard"_el, "wait"_el, "join"_el, "release"_el}) {
            const auto child = el::Subprocess::create(
                el::Path{el::unittest::fh::unitTestExecutablePath()},
                el::StringList{"name:WorkerPoolFatalChild"_el, "+name:WorkerPoolFatalChild"_el, "--no-color"_el},
                el::SubprocessOptions{}.setEnvironmentChanges(
                    el::EnvironmentChanges{}.setVariable("ERBSLAND_WORKER_POOL_FATAL_TEST"_el, mode)));
            child->start();
            const auto status = child->wait(el::TimeDelta::seconds(10));
            if (!status) {
                child->kill();
            }
            REQUIRE(status.has_value());
            REQUIRE(status->exitCode().has_value());
            REQUIRE_EQUAL(*status->exitCode(), std::int32_t{73});
        }
    }

    SKIP_BY_DEFAULT()
    void testWorkerPoolFatalChild() {
        const auto mode = el::EnvironmentVariables{}.get("ERBSLAND_WORKER_POOL_FATAL_TEST"_el);
        if (!mode) {
            return;
        }
        std::set_terminate([]() -> void { std::_Exit(73); });
        auto pool = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(1));
        auto release = std::latch{1};
        auto future = pool->submit([owned = pool, mode, &release]() mutable -> void {
            std::set_terminate([]() -> void { std::_Exit(73); });
            release.wait();
            if (*mode == "memory"_el) {
                throw std::bad_alloc{};
            }
            if (*mode == "logic"_el) {
                throw el::LogicFailure{"Fatal test."_el};
            }
            if (*mode == "standard"_el) {
                throw std::runtime_error{"Unexpected exception."};
            }
            if (*mode == "wait"_el) {
                owned->wait();
            }
            if (*mode == "join"_el) {
                owned->join();
            }
            if (*mode == "release"_el) {
                owned.reset();
            }
        });
        pool.reset();
        release.count_down();
        future.wait();
        std::_Exit(75);
    }
};
