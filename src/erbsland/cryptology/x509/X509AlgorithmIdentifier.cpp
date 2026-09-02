// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "X509AlgorithmIdentifier.hpp"

#include "../impl/X509Parser.hpp"

#include "../../err/RuntimeError.hpp"

namespace erbsland::cryptology {

auto X509AlgorithmIdentifier::fromDer(const mem::ByteBlock &der) noexcept -> X509AlgorithmIdentifier {
    try {
        return fromDerOrThrow(der);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto X509AlgorithmIdentifier::fromDerOrThrow(const mem::ByteBlock &der) -> X509AlgorithmIdentifier {
    return impl::X509Parser::parseAlgorithmIdentifier(der);
}

}
