// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/EventLoopDriver.hpp>
#include <erbsland/Path.hpp>

#include <array>
#include <cstdio>
#include <optional>

namespace demo {

/// Waits for one input descriptor and a thread-safe wake pipe.
class PosixPipeDriver final : public el::EventLoopDriver {
public:
    /// Open the supplied file or borrow standard input for exclusive reading.
    /// @param file Optional input file.
    /// @throws el::RuntimeError If input or the wake pipe cannot be opened.
    explicit PosixPipeDriver(const std::optional<el::Path> &file);
    ~PosixPipeDriver() override;

    // defaults/deletions
    PosixPipeDriver(const PosixPipeDriver &) = delete;
    auto operator=(const PosixPipeDriver &) -> PosixPipeDriver & = delete;

public: // implement EventLoopDriver
    void wait() override;
    void wait(el::TimeDelta maximumWait) override;
    void wake() override;

public: // input
    /// Read once after poll signals input. Called only by the owner loop.
    /// @return No value until ready, an empty block at EOF, or an owned chunk.
    /// @throws el::RuntimeError If the native read fails.
    auto readReadyChunk() -> std::optional<el::ByteBlock>;

private:
    void pollOnce(int timeout);

private:
    std::unique_ptr<std::FILE, int (*)(std::FILE *)> _file{nullptr, &std::fclose}; ///< Owns optional file.
    int _input;                     ///< Borrowed descriptor. Disabled after EOF.
    std::array<int, 2> _wakePipe{}; ///< Nonblocking read and write ends.
    bool _inputReady{};             ///< Only the wait thread reads and writes this flag.
};

}
