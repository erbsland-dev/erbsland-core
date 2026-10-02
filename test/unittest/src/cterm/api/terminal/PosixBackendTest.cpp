// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/cterm/impl/PosixBackend.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <unistd.h>

#include <array>
#include <stdexcept>
#include <thread>

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
        [[nodiscard]] auto readDescriptor() const noexcept -> int { return _descriptors[0]; }
        [[nodiscard]] auto writeDescriptor() const noexcept -> int { return _descriptors[1]; }

    private:
        std::array<int, 2> _descriptors{-1, -1};
    };

    /// Redirect standard input to a bounded test pipe and restore its original mode.
    /// @notest{Test-local native input fixture.}
    class InputPipe final {
    public:
        InputPipe() :
            _savedInput{::dup(STDIN_FILENO)},
            _backend{el::cterm::impl::PosixBackend::getOrCreate(el::cterm::TerminalFlag::NoSignalHandling)},
            _originalMode{_backend->inputMode()} {
            if (_savedInput < 0 || ::dup2(_pipe.readDescriptor(), STDIN_FILENO) < 0) {
                throw std::runtime_error{"Failed to redirect test input."};
            }
            _backend->purgePendingInput();
            _backend->setInputMode(el::cterm::Input::Mode::Key);
        }
        ~InputPipe() {
            _backend->setInputMode(_originalMode);
            _backend->purgePendingInput();
            static_cast<void>(::dup2(_savedInput, STDIN_FILENO));
            static_cast<void>(::close(_savedInput));
        }
        InputPipe(const InputPipe &) = delete;
        InputPipe(InputPipe &&) = delete;
        auto operator=(const InputPipe &) -> InputPipe & = delete;
        auto operator=(InputPipe &&) -> InputPipe & = delete;
        [[nodiscard]] auto backend() const -> const el::cterm::BackendPtr & { return _backend; }
        void write(const char *text, std::size_t length) const {
            static_cast<void>(::write(_pipe.writeDescriptor(), text, length));
        }

    private:
        Pipe _pipe;
        int _savedInput;
        el::cterm::BackendPtr _backend;
        el::cterm::Input::Mode _originalMode;
    };

public:
    void testPipeIsNotInteractiveOutput() {
        const auto pipe = Pipe{};

        REQUIRE_FALSE(el::cterm::impl::PosixBackend::isInteractiveOutput(pipe.writeDescriptor()));
        REQUIRE_FALSE(el::cterm::impl::PosixBackend::isInteractiveOutput(-1));
    }
    void testNativePollingAndBoundedDeadline() {
        const auto input = InputPipe{};
        const auto start = el::time::TimePoint::now();
        REQUIRE_FALSE(input.backend()->readKey(el::time::Milliseconds{-5}).valid());
        REQUIRE_FALSE(input.backend()->readKey(el::time::Milliseconds{}).valid());
        REQUIRE_FALSE(input.backend()->readKey(el::time::Milliseconds{5}).valid());
        const auto elapsed = el::time::TimePoint::now() - start;
        REQUIRE(elapsed >= el::time::Milliseconds{3});
        input.write("a", 1);
        REQUIRE_EQUAL(input.backend()->readKey(el::time::Milliseconds{}), el::cterm::Key{U'a'});
    }
    void testIncompleteEscapeReturnsWithoutAKey() {
        const auto input = InputPipe{};
        input.write("\033[", 2);
        REQUIRE_FALSE(input.backend()->readKey(el::time::Milliseconds{5}).valid());
        // Native waits can overshoot under load, expiring the incomplete escape sequence before they return.
        // Deadline arithmetic is tested with an injected clock in InputTimeoutTest.
        input.backend()->purgePendingInput();
        input.write("\033[A", 3);
        REQUIRE_EQUAL(input.backend()->readKey(el::time::Milliseconds{}), el::cterm::Key{el::cterm::Key::Up});
    }
    void testPollingSkipsUnsupportedSequencesWithinTheCollectedInput() {
        const auto input = InputPipe{};
        input.write("\033[999~z", 7);
        REQUIRE_EQUAL(input.backend()->readKey(el::time::Milliseconds{}), el::cterm::Key{U'z'});
    }
    void testNativeWaitBlocksUntilInputArrives() {
        const auto input = InputPipe{};
        auto writer = std::jthread{[&input]() -> void {
            std::this_thread::sleep_for(el::time::TimeDelta{el::time::Milliseconds{5}}.toStdNanoseconds());
            input.write("b", 1);
        }};
        REQUIRE_EQUAL(input.backend()->waitForKey(), el::cterm::Key{U'b'});
    }
};
