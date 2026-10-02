// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EcdsaCase.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

EcdsaCase::EcdsaCase(ci::NistPrimeCurve::Name name, EcdsaOperation operation) : _name{name}, _operation{operation} {
    const auto id =
        el::StringFormat{"ecdsa/p{}/{}"_el}.build(name == ci::NistPrimeCurve::Name::P256 ? 256 : 384, operationName());

    configure(TestMetadata{id, "NIST curve secret arithmetic and deterministic signing"_el}, el::ByteLength{4096});
}

auto EcdsaCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<EcdsaCase>(_name, _operation);
}

auto EcdsaCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<EcdsaFixture> {
    return std::make_unique<EcdsaFixture>(random, population, _name, _operation);
}

auto EcdsaCase::operationName() const -> el::String {
    switch (_operation) {
    case EcdsaOperation::PublicKey:
        return "public-key"_el;
    case EcdsaOperation::Sign:
        return "sign"_el;
    case EcdsaOperation::Multiply:
        return "order-multiply"_el;
    case EcdsaOperation::Invert:
        return "order-invert"_el;
    case EcdsaOperation::Add:
        return "order-add"_el;
    case EcdsaOperation::BaseMultiply:
        return "base-multiply"_el;
    }
    throw el::LogicError{"Unknown scenario operation."_el};
}

}
