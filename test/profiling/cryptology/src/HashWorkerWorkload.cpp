// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "HashWorkerWorkload.hpp"

#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/profiling/WorkerExecutionContext.hpp>

namespace app::cryptology {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

HashWorkerWorkload::HashWorkerWorkload(
    const el::cryptology::HashAlgorithm algorithm, std::shared_ptr<const el::ByteBuffer> input, const bool failWorker) :
    _algorithm{algorithm}, _input{std::move(input)}, _failWorker{failWorker} {
}

auto HashWorkerWorkload::execute(const pf::WorkerExecutionContext &context) -> pf::WorkerMeasurement {
    if (_failWorker) {
        throw el::ApplicationError{el::String{"Requested hash worker failure."_el}};
    }
    auto digest = el::ByteBlock{};
    auto operations = std::uint64_t{};
    for (; operations < context.operations && !context.stopToken.stop_requested(); ++operations) {
        auto hasher = el::cryptology::Hasher{_algorithm};
        hasher.update(_input->span());
        digest = hasher.finalize();
    }
    return pf::WorkerMeasurement{
        .operations = operations,
        .metrics = el::List<std::uint64_t>{operations * _input->length().toRawValue()},
        .sink = digest.length().toRawValue(),
        .digest = std::move(digest)};
}

}
