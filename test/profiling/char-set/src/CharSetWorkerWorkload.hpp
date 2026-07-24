// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AllocationTracker.hpp"
#include "CharSetFixture.hpp"
#include "CharSetWorkerWorkload_fwd.hpp"
#include "Operation.hpp"

#include <erbsland/profiling/WorkerExecutionContext.hpp>
#include <erbsland/profiling/WorkerWorkload.hpp>

#include <limits>
#include <utility>

namespace app::charset {

/// Independent worker for one character-set operation.
/// @notest{Covered by the character-set profiling smoke tests.}
class CharSetWorkerWorkload final : public erbsland::profiling::WorkerWorkload {
public:
    /// Create a worker from prepared immutable inputs.
    CharSetWorkerWorkload(Operation operation, CharSetFixture fixture);

public: // implement WorkerWorkload
    [[nodiscard]] auto execute(const erbsland::profiling::WorkerExecutionContext &context)
        -> erbsland::profiling::WorkerMeasurement override;

private:
    template <typename Function>
    [[nodiscard]] auto measure(const erbsland::profiling::WorkerExecutionContext &context, Function function)
        -> erbsland::profiling::WorkerMeasurement {
        auto sink = context.seed;
        auto operations = std::uint64_t{};
        auto allocationScope = AllocationScope{_fixture.trackAllocations};
        for (; operations < context.operations && !context.stopToken.stop_requested(); ++operations) {
            function(operations, sink);
        }
        const auto allocations = allocationScope.finish();
        return erbsland::profiling::WorkerMeasurement{
            .operations = operations,
            .metrics =
                el::List<std::uint64_t>{
                    operations,
                    saturatedMultiply(operations, _fixture.characterCount),
                    allocations.allocations,
                    allocations.allocatedBytes,
                    allocations.deallocations},
            .sink = sink};
    }

    static void consumeSet(std::uint64_t &sink, const el::CharSet &value) noexcept;
    static void consumeBoolean(std::uint64_t &sink, bool value) noexcept;
    static void consumeSize(std::uint64_t &sink, std::uint64_t value) noexcept;
    [[nodiscard]] static auto saturatedMultiply(std::uint64_t first, std::uint64_t second) noexcept -> std::uint64_t;

private:
    Operation _operation;    ///< The measured operation.
    CharSetFixture _fixture; ///< Independent immutable worker inputs.
};

}
