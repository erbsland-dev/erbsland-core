#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/impl/worker_executor/WorkerAdmissionError.hpp>
#include <erbsland/util/impl/worker_executor/WorkerExecutor.hpp>
#include <erbsland/util/impl/worker_executor/WorkerServices.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <vector>

TESTED_TARGETS(WorkerExecutor WorkerServices WorkerAdmissionError)
class WorkerExecutorTest final : public el::UnitTest {
public:
    void testBurstsWakeAllWorkersAndReuseThePool() {
        auto executor = el::util::impl::WorkerExecutor{4U, 32U, 1U};
        for (auto round = 0; round < 8; ++round) {
            const auto release = std::make_shared<std::promise<void>>();
            const auto gate = release->get_future().share();
            auto started = std::vector<std::future<void>>{};
            auto finished = std::vector<std::future<void>>{};
            for (auto index = 0; index < 4; ++index) {
                const auto entering = std::make_shared<std::promise<void>>();
                const auto leaving = std::make_shared<std::promise<void>>();
                started.push_back(entering->get_future());
                finished.push_back(leaving->get_future());
                executor.submit([entering, leaving, gate]() -> void {
                    entering->set_value();
                    gate.wait();
                    leaving->set_value();
                });
            }
            auto concurrent = true;
            for (auto &entry : started) {
                concurrent = (entry.wait_for(std::chrono::seconds{2}) == std::future_status::ready) && concurrent;
            }
            // Release every worker before reporting a missed wake or loss of concurrency.
            release->set_value();
            for (auto &entry : finished) {
                REQUIRE_EQUAL(entry.wait_for(std::chrono::seconds{2}), std::future_status::ready);
            }
            REQUIRE(concurrent);
        }
    }

    void testSecurityCannotOccupyAllAdmission() {
        auto executor = el::util::impl::WorkerExecutor{2U, 2U, 1U};
        const auto security = executor.reserve(el::util::impl::WorkerExecutor::WorkClass::Security);
        REQUIRE_THROWS_AS(
            el::util::impl::WorkerAdmissionError,
            executor.reserve(el::util::impl::WorkerExecutor::WorkClass::Security));
        REQUIRE(executor.reserve().isValid());
    }

    void testAdmissionAndReservedCleanup() {
        auto executor = el::util::impl::WorkerExecutor{1U, 1U, 1U};
        auto reservation = executor.reserve();
        REQUIRE(reservation.isValid());
        REQUIRE_THROWS_AS(el::util::impl::WorkerAdmissionError, executor.reserve());
        const auto release = std::make_shared<std::promise<void>>();
        const auto gate = release->get_future().share();
        const auto started = std::make_shared<std::promise<void>>();
        auto start = started->get_future();
        reservation.submit([started, gate]() -> void {
            started->set_value();
            gate.wait();
        });
        const auto status = start.wait_for(std::chrono::seconds{2});
        const auto completed = std::make_shared<std::promise<void>>();
        auto completion = completed->get_future();
        const auto sequence = std::make_shared<std::atomic<int>>(0);
        reservation.submit([sequence]() -> void { sequence->store(1); });
        reservation.submit([sequence, completed]() -> void {
            sequence->fetch_add(1);
            completed->set_value();
        });
        release->set_value();
        REQUIRE_EQUAL(status, std::future_status::ready);
        REQUIRE_EQUAL(completion.wait_for(std::chrono::seconds{2}), std::future_status::ready);
        REQUIRE_EQUAL(sequence->load(), 2);
    }

    void testSecurityCannotOccupyAllWorkers() {
        auto executor = el::util::impl::WorkerExecutor{2U, 4U, 1U};
        const auto release = std::make_shared<std::promise<void>>();
        const auto gate = release->get_future().share();
        const auto started = std::make_shared<std::promise<void>>();
        auto start = started->get_future();
        executor.submit(
            [started, gate]() -> void {
                started->set_value();
                gate.wait();
            },
            el::util::impl::WorkerExecutor::WorkClass::Security);
        const auto status = start.wait_for(std::chrono::seconds{2});
        const auto secondStarted = std::make_shared<std::atomic<bool>>(false);
        executor.submit(
            [secondStarted, gate]() -> void {
                secondStarted->store(true);
                gate.wait();
            },
            el::util::impl::WorkerExecutor::WorkClass::Security);
        const auto completed = std::make_shared<std::promise<void>>();
        auto completion = completed->get_future();
        executor.submit([completed]() -> void { completed->set_value(); });
        const auto progress = completion.wait_for(std::chrono::seconds{2});
        const auto prematurelyStarted = secondStarted->load();
        release->set_value();
        REQUIRE_EQUAL(status, std::future_status::ready);
        REQUIRE_EQUAL(progress, std::future_status::ready);
        REQUIRE_FALSE(prematurelyStarted);
    }

    void testReservationsSurviveExecutorDestruction() {
        auto reservation = el::util::impl::WorkerReservation{};
        {
            auto executor = el::util::impl::WorkerExecutor{1U, 1U, 1U};
            reservation = executor.reserve();
        }
        const auto completed = std::make_shared<std::promise<void>>();
        auto completion = completed->get_future();
        reservation.submit([completed]() -> void { completed->set_value(); });
        REQUIRE_EQUAL(completion.wait_for(std::chrono::seconds{2}), std::future_status::ready);
    }

    void testReleasedAdmissionCanBeReused() {
        auto executor = el::util::impl::WorkerExecutor{1U, 1U, 1U};
        {
            const auto reservation = executor.reserve();
            REQUIRE(reservation.isValid());
        }
        REQUIRE(executor.reserve().isValid());
    }

    void testExecutionDomainsAreIndependent() {
        REQUIRE_NOT_EQUAL(&el::util::impl::worker_services::io(), &el::util::impl::worker_services::blocking());
        REQUIRE_NOT_EQUAL(&el::util::impl::worker_services::io(), &el::util::impl::worker_services::coroutine());
        REQUIRE_NOT_EQUAL(&el::util::impl::worker_services::blocking(), &el::util::impl::worker_services::coroutine());
    }

    void testWaitingCoroutineDoesNotStarveStreamWorker() {
        auto coroutine = el::util::impl::WorkerExecutor{1U, 1U, 1U};
        auto stream = el::util::impl::WorkerExecutor{1U, 1U, 1U};
        const auto completed = std::make_shared<std::promise<bool>>();
        auto completion = completed->get_future();
        const auto streamWork = stream.reserve();
        coroutine.submit([completed, streamWork]() -> void {
            const auto written = std::make_shared<std::promise<void>>();
            auto writeCompletion = written->get_future();
            streamWork.submit([written]() -> void { written->set_value(); });
            completed->set_value(writeCompletion.wait_for(std::chrono::seconds{2}) == std::future_status::ready);
        });
        REQUIRE_EQUAL(completion.wait_for(std::chrono::seconds{3}), std::future_status::ready);
        REQUIRE(completion.get());
    }
};
