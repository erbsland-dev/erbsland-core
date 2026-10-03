// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "WorkerPoolDemos.hpp"

#include <erbsland/EventLoop.hpp>
#include <erbsland/List.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/Utilities.hpp>

namespace demo {

/// Deliver parallel projections on one event loop without waiting inside its callbacks.
/// Keep the pool alive until its final event; quit closes admission while accepted
/// computations and their completion callbacks finish normally.
void eventDelivery() {
    const auto loop = el::EventLoop::create();
    auto workers = el::WorkerPoolPtr{};

    loop->invoke([&]() -> void {
        workers = loop->get<el::Utilities>().createWorkerPool(el::WorkerPoolOptions{}.setWorkerCount(2));
        workers->events().onFinal([&]() -> void { loop->quit(); });
        workers->map(
            el::List<double>{100.0, 250.0, 500.0},
            [](const double &initial) -> double {
                auto population = initial;
                for (auto year = 0; year < 20; ++year) {
                    population += 0.15 * population * (1.0 - population / 1000.0);
                }
                return population;
            },
            [](el::List<double> populations) -> void {
                el::stdOut()->printLine("Habitat: "_el, "duingebied"_el);
                for (const auto population : populations) {
                    el::stdOut()->printLine(el::StringFormat{"Projected population: {:.1f}"_el}.build(population));
                }
            });
        workers->quit();
    });

    loop->run();
    workers->join();
}

}
