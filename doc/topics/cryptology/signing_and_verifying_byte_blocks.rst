..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Signed Byte Blocks
    single: Digital Signatures; Byte Blocks
    single: Ed25519; Signed Records

*********************************
Signing and Verifying Byte Blocks
*********************************

Suppose a puzzle publisher distributes a score record that readers may copy or store outside the publisher's system.
The readers need to know that the bytes came from the publisher and have not changed.
A :cpp:class:`SignedByteBlock <erbsland::cryptology::SignedByteBlock>` packages the bytes, their intended use, and a
signature into one binary record.
Readers need only the publisher's public key to check it.

This page walks through the signing and verification workflow, then opens the binary format so you can exchange these
records with an implementation in another language.
The signature authenticates bytes; it does not hide them, establish when they were signed, or prevent a valid record
from being replayed.
If those properties matter, add a suitable channel or protocol around the signed record.

Create and Share the Keys
=========================

For this format, generate a :cpp:class:`SigningPrivateKey <erbsland::cryptology::SigningPrivateKey>` with the
:cpp:enumerator:`Ed25519 <erbsland::cryptology::SigningKeyProfile::Ed25519>` profile.
Other signing profiles serve other APIs, but ``SignedByteBlock`` currently signs and verifies Ed25519 records.
Do not rely on the key generator's default profile, which is different.

Keep the private key with the publisher and give each reader the matching
:cpp:class:`PublicKey <erbsland::cryptology::PublicKey>` through a trusted provisioning step.
The public key can be stored as ``PUBLIC KEY`` PEM; the private key can be stored as ``PRIVATE KEY`` PEM or encrypted
PKCS#8. The in-memory round trip below shows the two roles without printing the private material.
For persistent keys, use the key classes' file methods and protect the private key file against disclosure.
Generate a long-lived pair once for an identity; generating a new pair for every record would leave readers without a
stable verifier.

.. erbsland-demo::
    :source: cryptology/SignedByteBlocks/CreateKeys.cpp
    :exec: cryptology/signed_byte_blocks --demo CreateKeys
    :source-sha256: 60ee0c7ff7a9b066e100aca15ec190398e76b81c4b8b00bd322b917a36ddd10b

.. code-block:: cpp

    /// Generate an Ed25519 signing key and distribute its public verifier separately.
    ///
    /// A signed score record can be checked by readers who have the public key. Keep the private PKCS#8 key with the
    /// score publisher; the public SubjectPublicKeyInfo key may be shared with every verifier. PEM conversion here models
    /// separate storage without writing a private key or its text to the console.
    void createKeys() {
        // Generate this pair once for the publisher and export the two roles separately.
        auto signingKey = el::cryptology::SigningPrivateKey::generate(el::cryptology::SigningKeyProfile::Ed25519);
        const auto privatePem = signingKey.toPem();
        const auto publicPem = signingKey.publicKey().toPem();

        // Each side imports only the key it needs.
        const auto publisherKey = el::cryptology::SigningPrivateKey::fromPemOrThrow(privatePem);
        const auto readerKey = el::cryptology::PublicKey::fromPemOrThrow(publicPem);
        el::io::printLine("Publisher and reader keys match: "_el, publisherKey.matches(readerKey));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Publisher and reader keys match: true

.. erbsland-demo-end::

Sign the Bytes for One Purpose
==============================

Convert the application value to bytes before signing.
For text, choose an exact encoding such as UTF-8; a verifier compares the bytes it receives, not an equivalent rendering
of the text.
Pass those bytes and a purpose string to ``sign()``.
The purpose is a small protocol name that separates uses of the same key: a score record should not be accepted where an
application expects a level description.
Choose a stable, distinct purpose for each kind of record and include a version when the meaning may evolve.

The optional key ID, ``score_2026`` here, helps a reader find a candidate public key among several configured keys.
It is a lookup hint, not proof of identity.
After signing, ``toByteBlock()`` returns the exact binary record to store or transmit.

.. erbsland-demo::
    :source: cryptology/SignedByteBlocks/SignAndVerify.cpp
    :exec: cryptology/signed_byte_blocks --demo SignAndVerify
    :source-sha256: 1324fff93b5069e4a34f907f68a77520ff25f0fcb82099f39709810217499350

.. code-block:: cpp

    /// Sign a complete score record with a purpose reserved for this record type.
    ///
    /// The `SignedByteBlock` stores the exact payload bytes, a purpose, an optional key lookup hint, and an Ed25519
    /// signature. The serialized block is self-contained and can be stored or sent without the private key.
    void signAndVerify() {
        const auto signingKey = el::cryptology::SigningPrivateKey::generate(el::cryptology::SigningKeyProfile::Ed25519);
        const auto scoreText = el::String{"Poäng: 42; nivå: skog"_el};
        const auto score = el::StringEncoder{scoreText}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);

        // Bind the score bytes to this application use and include a public lookup hint.
        const auto signedScore =
            el::cryptology::SignedByteBlock::sign(signingKey, score.span(), "puzzle.score/v1"_el, "score_2026"_el);
        const auto storedRecord = signedScore.toByteBlock();
        el::io::printLine("Serialized signed score bytes: "_el, storedRecord.length());
        el::io::printLine(
            "Publisher verifies its copy: "_el,
            signedScore.verify(signingKey.publicKey(), "puzzle.score/v1"_el).has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Serialized signed score bytes: 124
    Publisher verifies its copy: true

.. erbsland-demo-end::

Accept a Record Only After Verification
=======================================

A received record is untrusted even if its header and lengths look valid.
Parse it with a finite maximum payload length chosen for your application; this prevents an oversized embedded value
from being accepted merely because its encoding is well formed.
``fromByteBlockOrThrow()`` reports malformed input with an exception, while ``fromByteBlock()`` returns an empty value
on failure.
Only a nonempty parsed record can be verified.

If you maintain several public keys, ``untrustedKeyIdHint()`` can narrow the lookup to one candidate.
Never use that hint, or any payload bytes, as authenticated information before ``verify()`` succeeds.
Pass the trusted candidate key and the exact purpose your application expects.
On success, ``verify()`` returns the payload; on a wrong key, wrong purpose, or invalid signature, it returns no value.
The example keeps the key in memory to focus on the verification boundary; a reader normally loads its already
provisioned public key.

.. erbsland-demo::
    :source: cryptology/SignedByteBlocks/VerifyRecord.cpp
    :exec: cryptology/signed_byte_blocks --demo VerifyRecord
    :source-sha256: 756116f0867e0e7d2655cda8eda16dac7a4c3ab9626152e2533aa8ace37fa268

.. code-block:: cpp

    /// Parse an untrusted signed record under a payload limit, then verify before using its contents.
    ///
    /// Parsing only checks the binary format. The key hint is available for finding a candidate public key, but neither
    /// it nor the payload is trusted until `verify()` succeeds with the application's expected purpose.
    void verifyRecord() {
        const auto signingKey = el::cryptology::SigningPrivateKey::generate(el::cryptology::SigningKeyProfile::Ed25519);
        const auto scoreText = el::String{"Poäng: 42; nivå: skog"_el};
        const auto score = el::StringEncoder{scoreText}.encode(el::StringEncoding::Utf8, el::StringBomMode::Reject);
        const auto receivedBytes =
            el::cryptology::SignedByteBlock::sign(signingKey, score.span(), "puzzle.score/v1"_el, "score_2026"_el)
                .toByteBlock();

        // Treat bytes from storage or a peer as untrusted, even if the record parses.
        const auto record =
            el::cryptology::SignedByteBlock::fromByteBlockOrThrow(receivedBytes.span(), el::ByteLength{256U});
        // The hint selects a candidate from a trusted local key registry; it is not proof by itself.
        if (!record.untrustedKeyIdHint().has_value() || *record.untrustedKeyIdHint() != "score_2026"_el) {
            throw el::ApplicationError{"Unknown score publisher key."_el};
        }
        const auto candidateKey = signingKey.publicKey();
        const auto verifiedScore = record.verify(candidateKey, "puzzle.score/v1"_el);
        el::io::printLine("Verified score matches source: "_el, verifiedScore.has_value() && *verifiedScore == score);
        el::io::printLine("Wrong purpose accepted: "_el, record.verify(candidateKey, "puzzle.level/v1"_el).has_value());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Verified score matches source: true
    Wrong purpose accepted: false

.. erbsland-demo-end::

The trust decision also depends on your application.
Verification proves that the holder of the private key signed this exact record for this purpose.
Your key registry must establish whose key it is, and your protocol must decide whether an old but valid record is still
acceptable.
If a score can be replaced or revoked, include an application-level record identifier or revision in the payload and
check it after signature verification.

The Serialized Format
=====================

The binary record is a concatenation of fields with no separators or padding.
All lengths count **bytes**, not characters.
The first 12 bytes form a fixed header; the variable fields follow in the order stated by that header, and the final 64
bytes are the signature.

.. code-block:: text

    offset  size             content
    0       4                ASCII "ELSB"
    4       1                format version: 0x01
    5       1                algorithm:      0x01 (Ed25519)
    6       1                key-ID byte length K (0..64)
    7       1                purpose byte length P (1..255)
    8       4                payload byte length N (unsigned, big-endian)
    12      K                ASCII key ID, absent when K = 0
    12+K    P                UTF-8 purpose
    12+K+P  N                payload, unchanged
    12+K+P+N  64              Ed25519 signature

For example, a reader first checks the four magic bytes and the two version/algorithm bytes.
It then reads ``K``, ``P``, and the four-byte ``N`` from the header.
Those lengths tell it where the three variable fields end and where the 64-byte signature begins.
The complete record length must be exactly ``76 + K + P + N`` bytes: 12 header bytes plus 64 signature bytes plus the
three variable fields.
In the score demo, ``score_2026`` uses 10 bytes, ``puzzle.score/v1`` uses 15, and the UTF-8 score uses 23. The resulting
record is ``76 + 10 + 15 + 23 = 124`` bytes, matching the demo output.
An empty payload is valid; an empty purpose is not.

The key ID is either absent or one to 64 ASCII bytes matching ``[_a-z0-9]{1,64}``.
The purpose is one to 255 bytes of valid UTF-8, encoded without a byte-order mark.
The payload is arbitrary binary data and is limited both by its unsigned 32-bit length field and by the verifier's
explicit maximum.
Reject truncated fields, extra trailing bytes, invalid text, unsupported versions or algorithms, and lengths that do not
reproduce the exact record size.
Do these structural checks before interpreting any field as trusted data.

The signature is **pure Ed25519** over every byte before the signature itself, starting with ``ELSB`` and ending with
the final payload byte.
The signed message therefore includes the version, algorithm, lengths, key ID, and purpose as well as the payload.
Do not sign only the payload, reinterpret the lengths in native byte order, add a terminator, or hash the frame first
for an Ed25519 prehash variant.
A counterpart in Rust or Python can construct that exact prefix, sign it with an Ed25519 private key, append the 64-byte
signature, and verify the same prefix with the trusted public key.
After cryptographic verification, compare the purpose with the exact application purpose expected for that operation.

The parser can expose the key ID before verification to help select a key, but the key ID does not certify its own
meaning.
An attacker can write any syntactically valid key ID into an unsigned or forged record.
Select a candidate from a trusted key registry, verify the full frame, and only then associate the authenticated payload
with that key's identity.

For the API signatures and exception behavior, see :doc:`/reference/cryptology/signed_byte_blocks`.
