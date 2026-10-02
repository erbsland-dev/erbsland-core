// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Registry.hpp"

#include "cases/comparison/ComparisonCase.hpp"

namespace app::constant_time {

using namespace el::text::literals;

void Registry::addComparisons() {
    for (const auto size : {std::size_t{32}, std::size_t{256}, std::size_t{4096}}) {
        for (const auto mode : {ComparisonMode::Equal, ComparisonMode::MismatchPosition, ComparisonMode::FixedRandom}) {
            add<ComparisonCase>(size, mode, ComparisonContainer::Buffer);
            add<ComparisonCase>(size, mode, ComparisonContainer::Block);
            if (size == 32) {
                add<ComparisonCase>(size, mode, ComparisonContainer::Array);
            }
        }
    }
}

}
