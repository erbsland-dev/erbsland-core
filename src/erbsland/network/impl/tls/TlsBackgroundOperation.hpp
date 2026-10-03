#pragma once

#include "../../../core/impl/ApplicationDataAccess.hpp"
#include "../../../core/impl/ApplicationExecutionContext.hpp"
#include "../../../err/LogicFailure.hpp"
#include "../../../err/RuntimeError.hpp"
#include "../../../event/Events.hpp"
#include "../../../event/impl/EventContinuation.hpp"
#include "../../../text/Literals.hpp"
#include "../../../time/TimePoint.hpp"
#include "../../../util/impl/worker_executor/WorkerExecutor.hpp"
#include "../../../util/impl/worker_executor/WorkerServices.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <utility>

namespace erbsland::network::impl {

/// One logically cancellable native TLS operation on the shared bounded blocking domain.
/// Admission remains reserved until owner-loop completion or discarded-result cleanup.
/// @tested{TlsBackgroundOperationTest TlsClientConnectionTest TlsServerConnectionTest}
class TlsBackgroundOperation final {
public:
    // defaults/deletions
    TlsBackgroundOperation() = default;
    TlsBackgroundOperation(const TlsBackgroundOperation &) = delete;
    auto operator=(const TlsBackgroundOperation &) -> TlsBackgroundOperation & = delete;

    /// Discard delivery without waiting for a native operation.
    ~TlsBackgroundOperation() { cancel(); }

    /// Test whether a request is waiting or running.
    [[nodiscard]] auto isPending() const noexcept -> bool { return _cancelled != nullptr; }
    /// Discard queued or late results without waiting for uninterruptible native work.
    void cancel() noexcept {
        if (_cancelled != nullptr) {
            _cancelled->store(true);
            _cancelled.reset();
        }
    }
    /// Reserve work and completion capacity before scheduling a copied request.
    /// @throws util::impl::WorkerAdmissionError If the bounded security domain is full.
    template <typename Request, typename Completion>
    void start(Request request, event::EventsPtr events, time::TimePoint deadline, Completion completion) {
        if (isPending()) {
            using namespace text::literals;
            throw err::LogicFailure{"A TLS background operation is already pending."_el};
        }
        const auto applicationData = core::impl::applicationData();
        auto admission =
            util::impl::worker_services::blocking().reserve(util::impl::WorkerExecutor::WorkClass::Security);
        const auto continuation = event::impl::EventContinuation::create(events);
        auto cancelled = std::make_shared<std::atomic<bool>>(false);
        admission.submit(
            [request = std::move(request),
                applicationData,
                continuation,
                deadline,
                completion = std::move(completion),
                admission,
                cancelled]() mutable -> void {
                if (cancelled->load()) {
                    return;
                }
                auto result = std::optional<decltype(request.execute())>{};
                auto diagnostic = text::String{};
                if (time::TimePoint::now() < deadline) {
                    try {
                        const auto context = core::impl::ApplicationExecutionContext{applicationData};
                        result = request.execute();
                    } catch (const err::RuntimeError &error) {
                        diagnostic = error.reason();
                    }
                }
                if (cancelled->load()) {
                    return;
                }
                continuation->invoke(
                    [result = std::move(result),
                        diagnostic = std::move(diagnostic),
                        completion = std::move(completion),
                        admission,
                        cancelled]() mutable -> void {
                        if (!cancelled->load()) {
                            completion(std::move(result), std::move(diagnostic));
                        }
                    });
            });
        _cancelled = std::move(cancelled);
    }

private:
    std::shared_ptr<std::atomic<bool>> _cancelled; ///< Shared cancellation without retaining the connection.
};

}
