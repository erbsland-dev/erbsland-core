// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PiWorker.hpp"

#include <erbsland/Char.hpp>
#include <erbsland/CpCount.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/RuntimeError.hpp>
#include <erbsland/StringEditor.hpp>

#include <chrono>
#include <utility>

namespace demo {

using namespace el::text::literals;

PiWorker::PiWorker(Advance advance) : _advance{std::move(advance)} {
}

PiWorker::~PiWorker() {
    requestStop();
    join();
}

void PiWorker::start(std::optional<el::CpCount> limit, OnDigits onDigits, OnFinished onFinished) {
    if (_started || !onDigits || !onFinished) {
        throw el::LogicFailure{"A calculation worker starts once with both callbacks."_el};
    }
    _requested = true;
    _thread = std::thread{[this, limit, onDigits = std::move(onDigits), onFinished = std::move(onFinished)]() -> void {
        run(limit, onDigits, onFinished);
    }};
    _started = true;
}

void PiWorker::requestNextBatch() {
    if (!_started) {
        throw el::LogicFailure{"Cannot request work before starting the calculation."_el};
    }
    const auto lock = std::lock_guard{_mutex};
    if (_stop) {
        return;
    }
    if (_requested) {
        throw el::LogicFailure{"Only one calculation batch may be requested."_el};
    }
    _requested = true;
    _demand.notify_one();
}

void PiWorker::requestStop() noexcept {
    // Take the demand mutex so cancellation cannot arrive between the predicate check and waiting.
    const auto lock = std::lock_guard{_mutex};
    _stop = true;
    _demand.notify_one();
}

void PiWorker::join() {
    if (_thread.joinable()) {
        _thread.join();
    }
}

auto PiWorker::waitForDemand() -> bool {
    auto lock = std::unique_lock{_mutex};
    _demand.wait(lock, [this]() -> bool { return _requested || _stop; });
    if (_stop) {
        return false;
    }
    _requested = false;
    return true;
}

void PiWorker::run(const std::optional<el::CpCount> limit, const OnDigits &onDigits, const OnFinished &onFinished) {
    std::exception_ptr error;
    try {
        calculate(limit, onDigits);
    } catch (const el::RuntimeError &) {
        error = std::current_exception();
    }
    // Required completion is independent of optional display delivery. Submission failure is not hidden.
    onFinished(std::move(error));
}

void PiWorker::calculate(const std::optional<el::CpCount> limit, const OnDigits &onDigits) {
    auto produced = el::CpCount{0};
    auto integerSeen = false;
    while ((!limit || produced < *limit) && waitForDemand()) {
        auto batch = el::StringEditor{};
        auto batchCount = el::CpCount{0};
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds{20};
        while (!_stop && batchCount < el::CpCount{32U} && (!limit || produced < *limit)) {
            std::optional<el::Char> digit;
            if (_advance) {
                digit = _advance();
            } else {
                digit = _generator.advance();
            }
            if (digit) {
                if (!integerSeen) {
                    if (*digit != U'3') {
                        throw el::LogicFailure{"The integer part of pi must be 3."_el};
                    }
                    integerSeen = true;
                } else {
                    batch.append(*digit);
                    ++batchCount;
                    ++produced;
                }
            }
            if (!batch.isEmpty() && std::chrono::steady_clock::now() >= deadline) {
                break;
            }
        }
        if (!batch.isEmpty()) {
            onDigits(batch);
        }
    }
}

}
