// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaDecoding.hpp"

#include "RsaSignature.hpp"

#include "../../../../err/ParseError.hpp"
#include "../../SecureEraseGuard.hpp"

namespace erbsland::cryptology::impl::rsa_signer::decoder {

using namespace mem;
using namespace text;
using rsa_signature::Number;

[[noreturn]] void fail(const String &reason) {
    throw err::ParseError{reason};
}

[[nodiscard]] auto parseInteger(const Asn1Node &node, const String &name, const std::size_t count, const bool sensitive)
    -> Number {
    auto bytes = rsa_signature::decodePositiveInteger(node, name);
    if (sensitive) {
        bytes.markAsSensitive();
    }
    const auto bytesEraseGuard = SecureEraseGuard{bytes};
    return Number::fromBigEndian(bytes.span(), count);
}

}
