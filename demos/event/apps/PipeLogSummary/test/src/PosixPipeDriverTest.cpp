// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "../../src/PosixPipeDriver.hpp"

#include <erbsland/PathContent.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <thread>

TESTED_TARGETS(PosixPipeDriver)
class PosixPipeDriverTest final : public el::UnitTest {
public:
    void testWakeBeforeIndefiniteWait() {
        using namespace el::text::literals;
        const auto directory = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto path = directory->path() / "empty.log"_el;
        path.content().writeTextOrThrow(""_el);
        auto driver = demo::PosixPipeDriver{path};
        driver.wait(el::TimeDelta::zero());
        const auto eof = driver.readReadyChunk();
        REQUIRE(eof && eof->isEmpty()); // Now only the wake pipe can release wait().
        driver.wake();
        driver.wake();
        driver.wait();
        REQUIRE_FALSE(driver.readReadyChunk().has_value());
        auto producer = std::jthread{[&driver]() -> void { driver.wake(); }};
        driver.wait();
    }
};
