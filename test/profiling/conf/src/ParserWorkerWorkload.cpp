// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ParserWorkerWorkload.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/profiling/WorkerExecutionContext.hpp>

namespace app::conf {

namespace pf = erbsland::profiling;

using namespace el::text::literals;

ParserWorkerWorkload::ParserWorkerWorkload(
    el::StringList documents, const std::uint64_t corpusBytes, const bool failWorker) :
    _documents{std::move(documents)}, _corpusBytes{corpusBytes}, _failWorker{failWorker} {
}

auto ParserWorkerWorkload::execute(const pf::WorkerExecutionContext &context) -> pf::WorkerMeasurement {
    if (_failWorker) {
        throw el::ApplicationError{el::String{"Requested parser worker failure."_el}};
    }
    auto parser = el::conf::Parser{};
    auto sink = std::uint64_t{};
    auto operations = std::uint64_t{};
    for (; operations < context.operations && !context.stopToken.stop_requested(); ++operations) {
        for (const auto &text : _documents) {
            sink ^= parser.parseTextOrThrow(text)->size();
        }
    }
    return pf::WorkerMeasurement{
        .operations = operations,
        .metrics = el::List<std::uint64_t>{operations * _documents.count().toRawValue(), operations * _corpusBytes},
        .sink = sink};
}

}
