// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/random/impl/PosixEntropySource.hpp>
#include <erbsland/RandomFailure.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstddef>

using el::random::RandomFailure;

TESTED_TARGETS(PosixEntropySource PosixFileDescriptor RandomFailure)
class PosixEntropySourceTest final : public el::UnitTest {
public:
    void testFailureForMissingDevice() {
        auto source = el::random::impl::PosixEntropySource{"/this/path/does/not/exist"};
        auto bytes = std::array<std::byte, 1>{};

        REQUIRE_THROWS_AS(RandomFailure, source.fillBytes(bytes));
    }

    void testEmptyRequestDoesNotOpenDevice() {
        auto source = el::random::impl::PosixEntropySource{"/this/path/does/not/exist"};

        REQUIRE_NOTHROW(source.fillBytes({}));
    }
};
