// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PiDigitGenerator.hpp"

#include <erbsland/Char.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/String.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace demo {

/// Calculate one requested batch at a time on an ordinary thread, with cooperative cancellation.
/// The owner retains this worker until join; callbacks are independent of the event and terminal APIs.
/// @tested{PiWorkerTest PiSessionTest}
class PiWorker final {
public:
    /// Receive one owned batch of fractional digits on the calculation thread.
    using OnDigits = std::function<void(el::String)>;
    /// Receive exactly one final notification; nullptr means ordinary completion or cancellation.
    using OnFinished = std::function<void(std::exception_ptr)>;
    /// Advance the arithmetic once, optionally producing a digit; used to inject failures in tests.
    using Advance = std::function<std::optional<el::Char>()>;

public:
    /// Create a worker using the exact spigot algorithm, or an injected advancement function.
    /// @param advance An alternate calculation step; empty selects PiDigitGenerator.
    explicit PiWorker(Advance advance = {});
    /// Request cancellation and join as a cleanup fallback; never destroy this worker on its own thread.
    ~PiWorker();

    // defaults
    PiWorker(const PiWorker &) = delete;
    PiWorker(PiWorker &&) = delete;
    auto operator=(const PiWorker &) -> PiWorker & = delete;
    auto operator=(PiWorker &&) -> PiWorker & = delete;

public: // lifecycle
    /// Start once with demand for the first batch.
    /// @param limit Optional number of fractional digits, excluding the integer part.
    /// @param onDigits Called for each batch, before final completion.
    /// @param onFinished Called when no further batches will be produced.
    void start(std::optional<el::CpCount> limit, OnDigits onDigits, OnFinished onFinished);
    /// Request on the owner thread one next batch after receiving the preceding batch; never accumulate requests.
    /// The worker must have started, and no other request may be pending.
    void requestNextBatch();
    /// Wake a waiting worker or stop arithmetic at its next cancellation point; safe from any thread.
    void requestStop() noexcept;
    /// Join a started worker from its owner thread; does not depend on main-loop dispatch.
    void join();

private:
    /// Report calculation failure or cancellation through one required final callback.
    void run(std::optional<el::CpCount> limit, const OnDigits &onDigits, const OnFinished &onFinished);
    /// Consume one request or stop without performing arithmetic while idle.
    [[nodiscard]] auto waitForDemand() -> bool;
    /// Advance arithmetic between cancellation checks and return bounded batches.
    void calculate(std::optional<el::CpCount> limit, const OnDigits &onDigits);

private:
    PiDigitGenerator _generator;     ///< Arithmetic state, touched only by the worker.
    Advance _advance;                ///< Optional injected arithmetic step.
    std::thread _thread;             ///< Thread joined before owner destruction.
    std::atomic<bool> _stop{false};  ///< Cancellation independent of event-loop progress.
    std::mutex _mutex;               ///< Protects demand and wakeup ordering.
    std::condition_variable _demand; ///< Sleeps between requested batches.
    bool _requested{false};          ///< At most one next batch request.
    bool _started{false};            ///< Owner-thread one-shot lifecycle flag.
};

}
