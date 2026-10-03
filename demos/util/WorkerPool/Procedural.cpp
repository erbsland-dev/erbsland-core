// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkerPoolDemos.hpp"

#include <erbsland/List.hpp>
#include <erbsland/StopToken.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/WorkerPool.hpp>

namespace demo {

/// Parallel population projections with one ordered result list.
/// WorkerPool owns its threads and drains accepted work on destruction. A map call
/// snapshots the input and returns one standard future for the complete operation.
void procedural() {
    const auto workers = el::WorkerPool::create(el::WorkerPoolOptions{}.setWorkerCount(2));
    const auto populations = el::List<double>{100.0, 250.0, 500.0};

    auto projected = workers->map(populations, [](const double &initial, el::StopToken stop) -> double {
        auto population = initial;
        for (auto year = 0; year < 20 && !stop.isStopRequested(); ++year) {
            population += 0.15 * population * (1.0 - population / 1000.0);
        }
        return population;
    });

    // Results retain input order even when individual projections finish out of order.
    el::stdOut()->printLine("Habitat: "_el, "duingebied"_el);
    for (const auto population : projected.get()) {
        el::stdOut()->printLine(el::StringFormat{"Projected population: {:.1f}"_el}.build(population));
    }
}

}
