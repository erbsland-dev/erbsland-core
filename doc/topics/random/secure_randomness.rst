..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Secure Randomness
    single: SecureRandom
    single: FastRandom
    single: CharSet
    single: RandomError
    single: Visible Tokens
    single: Binary Secrets
    single: Entropy
    single: Tokens
    single: Failure Behavior
    single: Logging and Display
    single: Cryptographic
    single: Secrets
    single: Salts
    single: Nonces
    single: Keys
    single: Operating System
    single: Entropy Source
    single: Predictable
    single: Security

.. _random-secure-randomness:

*****************
Secure Randomness
*****************

This page explains when and how to use :cpp:class:`SecureRandom <erbsland::random::SecureRandom>`.
You will learn which values need secure randomness, how to generate visible tokens and binary secrets, how failures are
reported, and where the boundary to higher-level cryptographic APIs begins.

:cpp:class:`SecureRandom <erbsland::random::SecureRandom>` is the generator for values that protect something.
It reads from the operating system entropy source instead of exposing a deterministic engine.
Use it when prediction, replay, or reuse would create a security problem.
For non-security simulations, procedural content, and randomized test data, use
:cpp:class:`FastRandom <erbsland::random::FastRandom>` instead.

When Secure Randomness Is Required
==================================

Use :cpp:class:`SecureRandom <erbsland::random::SecureRandom>` for values that protect access, identity, private data,
or cryptographic material.
This includes session identifiers, reset links, invitation tokens, API keys, generated passwords, salts, nonces, and
temporary secrets.

The important question is not whether the value looks random.
The important question is what happens if another person can guess it.
If a successful guess would allow access, impersonation, replay, data exposure, or protocol failure, use secure
randomness.

Do not use deterministic or explicitly seeded generators for secrets.
A reproducible sequence is useful for tests and simulations, but it is the wrong property for security-sensitive values.

Secure Randomness Demo
======================

The following demo generates a protected notebook code and a binary sealing key.
It prints only stable properties: token length and byte length.
Do not log real secrets in application code unless the surrounding security model explicitly requires it.

.. erbsland-demo::
    :source: random/RandomTopics/SecureTokens.cpp
    :exec: random/random_topics --demo SecureTokens
    :source-sha256: adb0116c82c369a77956920e2dea65bae047626b6e755161648aff6c842f62dc

.. code-block:: cpp

    /// `SecureRandom` is the generator for secrets and security boundaries.
    ///
    /// It draws bytes from the operating system entropy source and throws
    /// `RandomError` if secure random data is unavailable. Generate protocol-visible
    /// tokens with an explicit character set, and use protected values for secrets
    /// that do not have to be exposed.
    void secureTokens() {
        const auto tokenAlphabet = el::CharSet::fromPattern("A-Za-z0-9"_el);

        try {
            // Field notebooks use a protected access code and a binary sealing key.
            auto &secureRandom = el::application().secureRandom();
            const auto notebookCode = secureRandom.buildString(el::CpLength{32U}, tokenAlphabet);
            const auto sealingKey = secureRandom.buildByteBuffer(el::ByteLength{32U});

            el::io::printLine("Feltbog token length : "_el, notebookCode.characterLength().toSizeT());
            el::io::printLine("Binary key bytes     : "_el, sealingKey.length().toSizeT());
        } catch (const el::RandomError &error) {
            el::io::printLine("Secure randomness is unavailable: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Feltbog token length : 32

.. erbsland-demo-end::

Visible Tokens
==============

Use ``buildString()`` with an explicit :cpp:class:`CharSet <erbsland::text::CharSet>` when the generated value must be
shown to a user, copied into a form, placed into a URL, or carried by a text protocol.

Choose the alphabet for the system that will receive the token.
For example, a URL-safe token, a case-insensitive invitation code, and a human-readable recovery code may need different
character sets.
A smaller alphabet can make tokens easier to handle, but it also carries less information per character.
Increase the token length when you restrict the alphabet.

The requested string length is measured in Unicode code points, not bytes.
For protocol tokens and identifiers, prefer a small explicit ASCII character set unless the receiving system clearly
supports a wider alphabet.

Binary Secrets
==============

Use ``buildByteBuffer()`` when the value is consumed as mutable, directly owned binary data.
This is usually the better foundation for cryptographic keys, salts, nonces, and internal tokens because it preserves
all generated bytes.

Formatting secure bytes into text should be a separate decision.
Encode binary data only when the next system boundary requires text, for example when writing a token into JSON, a URL,
or a configuration file.
This keeps the generated secret independent from its display or transport format.

Use ``fillBytes()`` when you already own the destination buffer.
Use ``buildByteBuffer()`` for independent dynamic storage or ``buildByteBlock()`` for a read-only copy-on-write block.

Protected Generated Values
==========================

On ``SecureRandom``, :cpp:func:`Random::buildString() <erbsland::random::Random::buildString>` marks the UTF-8 allocation
before writing generated text.
The builder finds the widest UTF-8 encoding in the character set, checks the capacity arithmetic, reserves once, and
writes each selected character directly into that allocation.

Use :cpp:func:`Random::buildByteBuffer() <erbsland::random::Random::buildByteBuffer>` on ``SecureRandom`` for directly
owned binary secrets, or ``buildByteBlock()`` for shared read-only storage.
The secure generator selects sensitive writable storage before filling it.
If generation throws after partially writing either protected value, destruction securely erases the complete
allocation.

Failure Behavior
================

If the operating system entropy source cannot provide random bytes, the API throws
:cpp:class:`RandomError <erbsland::random::RandomError>`.
This is intentional.
A secure generator must not silently continue with predictable data.

All methods that draw random data, including inherited convenience helpers, may throw this exception.
Handle this failure at a boundary where you can stop the operation, report a startup problem, deny the request, or fail
the current security-sensitive action.

Do not fall back to :cpp:class:`FastRandom <erbsland::random::FastRandom>` for a secret.
A fallback to predictable data would turn a hard failure into a hidden security problem.

Logging and Display
===================

Treat generated secrets as sensitive data from the moment they are created.
Avoid logging complete tokens, keys, salts, nonces, session identifiers, and reset links.

For diagnostics, prefer stable properties such as length, character-set validation, byte count, or a boolean success
result.
If production diagnostics require correlation, use a design that does not reveal the secret itself, such as a separate
identifier or a carefully reviewed redaction policy.

What SecureRandom Does Not Do
=============================

:cpp:class:`SecureRandom <erbsland::random::SecureRandom>` provides secure random bytes and the same convenience value
helpers as other random generators.
It does not implement key derivation, password hashing, authenticated encryption, signature algorithms, certificate
handling, or protocol-specific nonce rules.

For password storage, use :cpp:class:`PasswordHasher <erbsland::cryptology::PasswordHasher>`.
It obtains a fresh salt from :cpp:class:`el::application().secureRandom() <erbsland::core::Application>` for every new
record, so application code must not generate or persist password salts separately.
