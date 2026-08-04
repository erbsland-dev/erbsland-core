// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tls13Hkdf.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../mem/Byte.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../mem/Endianness.hpp"
#include "../../../text/Literals.hpp"
#include "../../Hasher.hpp"
#include "../../Hkdf.hpp"

#include <cstdint>
#include <limits>

namespace erbsland::cryptology::impl {

using namespace text::literals;

Tls13Hkdf::Tls13Hkdf(const HashAlgorithm algorithm) : _algorithm{algorithm} {
    // RFC 8446 section 9.1: the supported TLS 1.3 suites bind this label operation to SHA-256 or SHA-384 only.
    if (_algorithm != HashAlgorithm::Sha2_256 && _algorithm != HashAlgorithm::Sha2_384) {
        throw err::ParameterError{"TLS 1.3 HKDF requires SHA-256 or SHA-384."_el, "algorithm"_el};
    }
}

auto Tls13Hkdf::expandLabel(
    const mem::ConstByteSpan secret,
    const mem::ConstByteSpan label,
    const mem::ConstByteSpan context,
    const unit::ByteLength outputLength) const -> mem::ByteBlock {
    static constexpr auto cTls13Prefix = mem::ByteArray<6U>{
        mem::Byte{'t'}, mem::Byte{'l'}, mem::Byte{'s'}, mem::Byte{'1'}, mem::Byte{'3'}, mem::Byte{' '}};

    // RFC 8446 section 7.1: uint16 length, one-byte prefixed complete label, and one-byte prefixed context are the
    // security-relevant representation bounds. Reject them before constructing attacker-influenced storage.
    const auto completeLabelLength = cTls13Prefix.span().size() + label.size();
    if (outputLength.toSizeT() > std::numeric_limits<uint16_t>::max() ||
        completeLabelLength > std::numeric_limits<uint8_t>::max() ||
        context.size() > std::numeric_limits<uint8_t>::max()) {
        throw err::ParameterError{"The TLS 1.3 HKDF label exceeds its wire representation bounds."_el, "label"_el};
    }

    auto info = mem::ByteBlockEditor{};
    info.reserve(unit::ByteLength{2U + 1U + completeLabelLength + 1U + context.size()});

    // RFC 8446 section 7.1: HkdfLabel.length is the requested output encoded as uint16 in network order.
    info.appendInteger<uint16_t>(static_cast<uint16_t>(outputLength.toSizeT()), mem::Endianness::Big);

    // RFC 8446 section 7.1: HkdfLabel.label is uint8 length || "tls13 " || Label, without a trailing NUL.
    info.append(mem::Byte{static_cast<uint8_t>(completeLabelLength)});
    info.append(cTls13Prefix.span());
    info.append(label);

    // RFC 8446 section 7.1: HkdfLabel.context is the exact opaque context with its uint8 length prefix.
    info.append(mem::Byte{static_cast<uint8_t>(context.size())});
    info.append(context);

    // RFC 8446 section 7.1: HKDF-Expand-Label delegates the encoded HkdfLabel to RFC 5869 HKDF-Expand.
    return Hkdf{_algorithm}.expand(secret, info.span(), outputLength);
}

auto Tls13Hkdf::deriveSecret(
    const mem::ConstByteSpan secret, const mem::ConstByteSpan label, const mem::ConstByteSpan transcriptHash) const
    -> mem::ByteBlock {
    // RFC 8446 section 7.1: Derive-Secret always uses Transcript-Hash(Messages) and produces Hash.length bytes.
    if (transcriptHash.size() != _algorithm.digestSize().toSizeT()) {
        throw err::ParameterError{"The TLS 1.3 transcript hash has the wrong length."_el, "transcriptHash"_el};
    }
    return expandLabel(secret, label, transcriptHash, _algorithm.digestSize());
}

auto Tls13Hkdf::emptyHash() const -> mem::ByteBlock {
    // RFC 8446 section 7.1: Derive-Secret(..., "derived", "") uses Transcript-Hash of the empty message sequence.
    return Hasher{_algorithm}.finalize();
}

}
