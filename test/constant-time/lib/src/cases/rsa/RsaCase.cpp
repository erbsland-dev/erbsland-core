// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RsaCase.hpp"

#include <erbsland/err/LogicError.hpp>
#include <erbsland/random/Random.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ByteLength.hpp>

namespace app::constant_time {

using namespace el::text::literals;

RsaCase::RsaCase(RsaOperation operation) : _operation{operation} {
    const auto id = identifier();

    configure(
        TestMetadata{
            id,
            "Pinned RSA-2048 secret arithmetic and PSS signing"_el,
            operation >= RsaOperation::SignSha256 ? "fixed-vs-random-message-and-salt"_el
                                                  : "fixed-vs-random-fixed-width-operands"_el},
        el::ByteLength{65536});
}

auto RsaCase::create([[maybe_unused]] const bool automatic) const -> std::unique_ptr<TestCase> {
    return std::make_unique<RsaCase>(_operation);
}

auto RsaCase::createFixture(el::Random &random, const bool population) -> std::unique_ptr<RsaFixture> {
    if (!_context) {
        _context = std::make_shared<RsaContext>();
    }
    return std::make_unique<RsaFixture>(random, population, _context, _operation);
}

auto RsaCase::identifier() const -> el::String {
    switch (_operation) {
    case RsaOperation::Reduce:
        return "rsa/secret-reduce"_el;
    case RsaOperation::Multiply:
        return "rsa/secret-multiply"_el;
    case RsaOperation::Subtract:
        return "rsa/secret-subtract"_el;
    case RsaOperation::Power:
        return "rsa/secret-power"_el;
    case RsaOperation::SignSha256:
        return "rsa/pss-sha256-sign"_el;
    case RsaOperation::SignSha384:
        return "rsa/pss-sha384-sign"_el;
    }
    throw el::LogicError{"Unknown scenario operation."_el};
}

}
