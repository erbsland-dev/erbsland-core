// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X25519Case.hpp"

#include <erbsland/ByteCount.hpp>
#include <erbsland/LogicFailure.hpp>
#include <erbsland/Random.hpp>
#include <erbsland/String.hpp>

namespace app::constant_time {

using namespace el::text::literals;

X25519Case::X25519Case(X25519Operation operation) : _operation{operation} {
    const auto id = identifier();

    configure(TestMetadata{id, "Curve25519 scalar and field operation"_el}, el::ByteCount{4096});
}

auto X25519Case::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<X25519Case>(_operation);
}

auto X25519Case::createFixture(el::Random &random, const bool population) -> std::unique_ptr<X25519Fixture> {
    return std::make_unique<X25519Fixture>(random, population, _operation);
}

auto X25519Case::identifier() const -> el::String {
    switch (_operation) {
    case X25519Operation::Agree:
        return "x25519/agree"_el;
    case X25519Operation::Multiply:
        return "x25519/field-multiply"_el;
    case X25519Operation::Invert:
        return "x25519/field-invert"_el;
    case X25519Operation::Square:
        return "x25519/field-square"_el;
    case X25519Operation::Swap:
        return "x25519/field-swap"_el;
    }
    throw el::LogicFailure{"Unknown scenario operation."_el};
}

}
