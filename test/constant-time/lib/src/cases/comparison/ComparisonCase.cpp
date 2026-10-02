// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ComparisonCase.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

ComparisonCase::ComparisonCase(std::size_t size, ComparisonMode mode, ComparisonContainer container) :
    _size{size}, _mode{mode}, _container{container} {
    const auto id = el::StringFormat{"compare/{}/{}/{}"_el}.build(containerName(), size, modeName());

    configure(
        TestMetadata{id, "Constant-time equal-length byte comparison"_el, populationName()},
        el::ByteLength{size * 4U + 1024U});
}

auto ComparisonCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<ComparisonCase>(_size, _mode, _container);
}

auto ComparisonCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<ComparisonFixture> {
    return std::make_unique<ComparisonFixture>(random, population, _size, _mode, _container);
}

auto ComparisonCase::containerName() const -> el::String {
    switch (_container) {
    case ComparisonContainer::Buffer:
        return "buffer"_el;
    case ComparisonContainer::Block:
        return "block"_el;
    case ComparisonContainer::Array:
        return "array"_el;
    }
    throw el::LogicError{"Unknown comparison configuration."_el};
}

auto ComparisonCase::modeName() const -> el::String {
    switch (_mode) {
    case ComparisonMode::Equal:
        return "equal"_el;
    case ComparisonMode::MismatchPosition:
        return "mismatch-position"_el;
    case ComparisonMode::FixedRandom:
        return "fixed-random"_el;
    }
    throw el::LogicError{"Unknown comparison configuration."_el};
}

auto ComparisonCase::populationName() const -> el::String {
    switch (_mode) {
    case ComparisonMode::Equal:
        return "equal-0x55-vs-equal-0x00"_el;
    case ComparisonMode::MismatchPosition:
        return "first-vs-last-mismatch"_el;
    case ComparisonMode::FixedRandom:
        return "fixed-vs-random-equal-content"_el;
    }
    throw el::LogicError{"Unknown comparison configuration."_el};
}

}
