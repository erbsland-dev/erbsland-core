// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cterm/impl/PosixBackend.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <unistd.h>

#include <array>
#include <stdexcept>

TESTED_TARGETS(PosixBackend)
class PosixBackendTest final : public el::UnitTest {
    class Pipe final {
    public:
        Pipe() {
            if (::pipe(_descriptors.data()) != 0) {
                throw std::runtime_error{"Failed to create a test pipe."};
            }
        }
        ~Pipe() {
            static_cast<void>(::close(_descriptors[0]));
            static_cast<void>(::close(_descriptors[1]));
        }

        // defaults/deletions
        Pipe(const Pipe &) = delete;
        Pipe(Pipe &&) = delete;
        auto operator=(const Pipe &) -> Pipe & = delete;
        auto operator=(Pipe &&) -> Pipe & = delete;

    public:
        [[nodiscard]] auto writeDescriptor() const noexcept -> int { return _descriptors[1]; }

    private:
        std::array<int, 2> _descriptors{-1, -1};
    };

public:
    void testPipeIsNotInteractiveOutput() {
        const auto pipe = Pipe{};

        REQUIRE_FALSE(el::cterm::impl::PosixBackend::isInteractiveOutput(pipe.writeDescriptor()));
        REQUIRE_FALSE(el::cterm::impl::PosixBackend::isInteractiveOutput(-1));
    }
};
