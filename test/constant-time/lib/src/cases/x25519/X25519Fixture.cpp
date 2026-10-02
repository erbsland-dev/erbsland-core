// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "X25519Fixture.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/mem/Byte.hpp>
#include <erbsland/mem/ByteBuffer.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteIndex.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

X25519Fixture::X25519Fixture(el::Random &random, const bool population, X25519Operation operation) :
    _operation{operation} {
    auto secret = input(random, population, 32);
    _secret = std::move(secret);
    _message = el::ByteBuffer{el::ByteLength{32}};
    _message.set(el::ByteIndex{}, el::Byte{9U});
    _field = ci::x25519::FieldElement::fromBytes(_secret.span());
    require(ci::x25519::FieldElement::multiply(_field, ci::x25519::FieldElement::one()).toBytes() == _field.toBytes());
    if (operation == X25519Operation::Agree) {
        require(ci::x25519::publicKey(_secret.span()) == ci::x25519::scalarMultiply(_secret.span(), _message.span()));
    }
}

template <X25519Operation tOperation>
auto X25519Fixture::sample() -> uint64_t {
    constexpr auto operation = tOperation;

    if constexpr (operation == X25519Operation::Agree) {
        return checksum(ci::x25519::scalarMultiply(_secret.span(), _message.span()).span());
    } else if constexpr (operation == X25519Operation::Multiply) {
        return checksum(ci::x25519::FieldElement::multiply(_field, ci::x25519::FieldElement::one()).toBytes().span());
    } else if constexpr (operation == X25519Operation::Invert) {
        return checksum(ci::x25519::FieldElement::invert(_field).toBytes().span());
    } else if constexpr (operation == X25519Operation::Square) {
        return checksum(ci::x25519::FieldElement::square(_field).toBytes().span());
    } else {
        auto left = _field;
        auto right = ci::x25519::FieldElement::one();
        ci::x25519::FieldElement::conditionalSwap(left, right, _secret.get(el::ByteIndex{}).toUInt8() & 1U);
        return checksum(left.toBytes().span());
    }
}

auto X25519Fixture::measure(const uint64_t repetitions) -> uint64_t {
    switch (_operation) {
    case X25519Operation::Agree:
        return repeat(repetitions, [this]() -> uint64_t { return sample<X25519Operation::Agree>(); });
    case X25519Operation::Multiply:
        return repeat(repetitions, [this]() -> uint64_t { return sample<X25519Operation::Multiply>(); });
    case X25519Operation::Invert:
        return repeat(repetitions, [this]() -> uint64_t { return sample<X25519Operation::Invert>(); });
    case X25519Operation::Square:
        return repeat(repetitions, [this]() -> uint64_t { return sample<X25519Operation::Square>(); });
    case X25519Operation::Swap:
        return repeat(repetitions, [this]() -> uint64_t { return sample<X25519Operation::Swap>(); });
    }
    throw el::LogicError{"Unknown sample operation."_el};
}

}
