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

The following demo generates a visible notebook code and a binary sealing key.
It prints only stable properties: token length, alphabet validation, and byte length.
Do not log real secrets in application code unless the surrounding security model explicitly requires it.

.. erbsland-demo::
    :source: random/RandomTopics/SecureTokens.cpp
    :exec: random_topics --demo SecureTokens
    :source-sha256: 44df9fba9a6a0308e0cba5f595f4c07f75f0d06779c8aaa010a6e5dcaef5a889

.. code-block:: cpp

    /// `SecureRandom` is the generator for secrets and security boundaries.
    ///
    /// It draws bytes from the operating system entropy source and throws
    /// `RandomError` if secure random data is unavailable. Generate protocol-visible
    /// tokens with an explicit character set, and prefer byte blocks for binary
    /// keys, salts, and nonces.
    void secureTokens() {
        const auto tokenAlphabet = el::CharSet::fromPattern("A-Za-z0-9"_el);
        const auto yesNo = el::BooleanFormat::yesNo();

        try {
            // Field notebooks use a visible code and a binary sealing key.
            auto &secureRandom = el::application().secureRandom();
            const auto notebookCode = secureRandom.buildString(el::CpLength{32U}, tokenAlphabet);
            const auto sealingKey = secureRandom.buildByteBlock(el::ByteLength{32U});

            el::io::printLine("Feltbog token length : "_el, notebookCode.characterLength().toSizeT());
            el::io::printLine("Allowed alphabet     : "_el, yesNo, notebookCode.containsOnly(tokenAlphabet));
            el::io::printLine("Binary key bytes     : "_el, sealingKey.length().toSizeT());
        } catch (const el::RandomError &error) {
            el::io::printLine("Secure randomness is unavailable: "_el, error.reason());
        }
    }

.. erbsland-ansi::
    :escape-char: ␛

    Feltbog token length : 32
    Allowed alphabet     : yes
    Binary key bytes     : 32

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

Use ``buildByteBlock()`` when the value is consumed as binary data.
This is usually the better foundation for cryptographic keys, salts, nonces, and internal tokens because it preserves
all generated bytes.

Formatting secure bytes into text should be a separate decision.
Encode binary data only when the next system boundary requires text, for example when writing a token into JSON, a URL,
or a configuration file.
This keeps the generated secret independent from its display or transport format.

Use ``fillBytes()`` when you already own the destination buffer.
Use ``buildByteBlock()`` when you want the API to allocate and return a new copy-on-write byte block.

Failure Behavior
================

If the operating system entropy source cannot provide random bytes, the API throws
:cpp:class:`RandomError <erbsland::err::RandomError>`.
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

For diagnostics, prefer stable properties such as length, character-set validation, byte count, or a boolean
success result.
If production diagnostics require correlation, use a design that does not reveal the secret itself, such as a separate
identifier or a carefully reviewed redaction policy.

What SecureRandom Does Not Do
=============================

:cpp:class:`SecureRandom <erbsland::random::SecureRandom>` provides secure random bytes and the same convenience value
helpers as other random generators.
It does not implement key derivation, password hashing, authenticated encryption, signature algorithms, certificate
handling, or protocol-specific nonce rules.
