// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteOutputStream.hpp>
#include <erbsland/Literals.hpp>

#include <new>

using namespace el::text::literals;

/// Controlled finalization failures without OS-dependent timing.
/// @tested{PathReplacementTest}
class ReplacementOutputStream final : public el::stream::ByteOutputStream {
public:
    [[nodiscard]] auto outputSettings() const noexcept -> const el::stream::OutputStreamSettings & override {
        return settings;
    }
    [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return lifecycle; }
    [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }
    [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
        return el::stream::StreamWaitStatus::Ready;
    }
    auto write(el::mem::ConstByteSpan) -> el::stream::StreamWriteStatus override {
        return el::stream::StreamWriteStatus::Success;
    }
    auto flush() -> el::stream::StreamWriteStatus override {
        ++flushCalls;
        if (unexpectedFlush) {
            throw std::bad_alloc{};
        }
        if (failFlush) {
            throw el::stream::StreamError{
                el::stream::StreamErrorContext{"Native flush failure"_el, "Deep flush cause"_el}};
        }
        if (flushTimeout) {
            return el::stream::StreamWriteStatus::Timeout;
        }
        return el::stream::StreamWriteStatus::Success;
    }
    auto close() -> el::stream::StreamCloseStatus override {
        if (unexpectedClose) {
            throw std::bad_alloc{};
        }
        if (failClose) {
            throw el::stream::StreamError{
                el::stream::StreamErrorContext{"Native close failure"_el, "Deep close cause"_el}};
        }
        if (closeTimeout) {
            lifecycle = el::stream::StreamState::Closing;
            return el::stream::StreamCloseStatus::Timeout;
        }
        lifecycle = el::stream::StreamState::Closed;
        return el::stream::StreamCloseStatus::Closed;
    }
    void abort() noexcept override { lifecycle = el::stream::StreamState::Closed; }

public:
    el::stream::OutputStreamSettings settings;                        ///< Controlled settings.
    el::stream::StreamState lifecycle{el::stream::StreamState::Open}; ///< Current state.
    bool unexpectedFlush{false};                                      ///< Unexpected allocation failure during flush.
    bool unexpectedClose{false};                                      ///< Unexpected allocation failure during close.
    bool failFlush{false};                                            ///< Throw during flush.
    bool failClose{false};                                            ///< Throw during close.
    bool flushTimeout{false};                                         ///< Return flush timeout.
    bool closeTimeout{false};                                         ///< Return close timeout.
    int flushCalls{0};                                                ///< Number of flush attempts.
};
