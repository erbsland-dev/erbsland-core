// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Ed25519Case.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

Ed25519Case::Ed25519Case(Ed25519Operation operation) : _operation{operation} {
    const auto id = identifier();

    configure(TestMetadata{id, "Ed25519 secret arithmetic and signing"_el}, el::ByteLength{4096});
}

auto Ed25519Case::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<Ed25519Case>(_operation);
}

auto Ed25519Case::createFixture(el::Random &random, const bool population) -> std::unique_ptr<Ed25519Fixture> {
    return std::make_unique<Ed25519Fixture>(random, population, _operation);
}

auto Ed25519Case::identifier() const -> el::String {
    switch (_operation) {
    case Ed25519Operation::PublicKey:
        return "ed25519/public-key"_el;
    case Ed25519Operation::Sign:
        return "ed25519/sign"_el;
    case Ed25519Operation::Reduce:
        return "ed25519/scalar-reduce"_el;
    case Ed25519Operation::Add:
        return "ed25519/scalar-add"_el;
    case Ed25519Operation::Multiply:
        return "ed25519/scalar-multiply"_el;
    case Ed25519Operation::BaseMultiply:
        return "ed25519/base-multiply"_el;
    case Ed25519Operation::FieldMultiply:
        return "ed25519/field-multiply"_el;
    case Ed25519Operation::FieldInvert:
        return "ed25519/field-invert"_el;
    }
    throw el::LogicError{"Unknown scenario operation."_el};
}

}
