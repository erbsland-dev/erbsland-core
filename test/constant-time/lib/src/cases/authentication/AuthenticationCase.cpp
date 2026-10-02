// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "AuthenticationCase.hpp"

#include <erbsland/cryptology/HashAlgorithm.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

AuthenticationCase::AuthenticationCase(el::HashAlgorithm algorithm, AuthenticationOperation operation) :
    _algorithm{algorithm}, _operation{operation} {
    const auto id = el::StringFormat{"{}/{}"_el}.build(operationName(), el::HashAlgorithm{algorithm}.toString());

    configure(
        TestMetadata{
            id,
            "HMAC or HKDF fixed-length operation"_el,
            operation == AuthenticationOperation::HmacVerify ? "first-vs-last-invalid-tag"_el
                                                             : "fixed-vs-random-key-material"_el},
        el::ByteLength{8192});
}

auto AuthenticationCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<AuthenticationCase>(_algorithm, _operation);
}

auto AuthenticationCase::createFixture(el::Random &random, const bool population)
    -> std::unique_ptr<AuthenticationFixture> {
    return std::make_unique<AuthenticationFixture>(random, population, _algorithm, _operation);
}

auto AuthenticationCase::operationName() const -> el::String {
    switch (_operation) {
    case AuthenticationOperation::HmacCalculate:
        return "hmac/calculate"_el;
    case AuthenticationOperation::HmacVerify:
        return "hmac/verify"_el;
    case AuthenticationOperation::HkdfExtract:
        return "hkdf/extract"_el;
    case AuthenticationOperation::HkdfExpand:
        return "hkdf/expand"_el;
    }
    throw el::LogicError{"Unknown scenario operation."_el};
}

}
