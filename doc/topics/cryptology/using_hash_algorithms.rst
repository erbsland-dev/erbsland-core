..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Using Hash Algorithms
    single: Cryptology
    single: Hash Algorithm
    single: Hasher
    single: Message
    single: Message Digest
    single: Collision Resistance
    single: Preimage Resistance
    single: Hash Storage
    single: Algorithm Selection
    single: Hash Security

.. _cryptology-using-hash-algorithms:

*********************
Using Hash Algorithms
*********************

This page explains how to calculate cryptographic hashes in Erbsland Core.
You will learn the essential terminology, how to hash data with a fixed algorithm, how to select an algorithm from
application requirements, and how to store and validate the result safely.

Understanding Hash Algorithms
=============================

A cryptographic hash algorithm accepts a *message* of arbitrary length and produces a fixed-size *message digest*, also
called a *hash value*.
The message is a sequence of bytes.
The same bytes and algorithm always produce the same digest, while even a small change to the message should produce an
unrelated result.

The security of a hash algorithm is commonly described through three properties:

``Preimage resistance``
    Given a digest, finding a message with that digest should be computationally infeasible.

``Second-preimage resistance``
    Given one message, finding a different message with the same digest should be computationally infeasible.

``Collision resistance``
    Finding any two different messages with the same digest should be computationally infeasible.

A hash is not encryption.
There is no key and no operation that recovers the original message from its digest.
A plain hash also does not prove who created a message.
It detects a change only when the expected digest comes from a trusted source.
Use a message-authentication or signature mechanism when an attacker could replace both the message and its digest.

See :doc:`supported_hash_algorithms` for the algorithms provided by the library and their current security status.

Hash with a Fixed Algorithm
===========================

Use a fixed :cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>` when a protocol or file format already
defines the algorithm.
Construct :cpp:class:`Hasher <erbsland::cryptology::Hasher>` with that algorithm, add every message part with
:cpp:func:`Hasher::update() <erbsland::cryptology::Hasher::update>`, and finish with
:cpp:func:`Hasher::finalize() <erbsland::cryptology::Hasher::finalize>`.

The following demo hashes one observation in two updates.
Both strings become one continuous byte message; update boundaries do not become part of the digest.

.. erbsland-demo::
    :source: cryptology/HashAlgorithms/FixedSha3.cpp
    :exec: cryptology/hash_algorithms --demo FixedSha3
    :source-sha256: 61d38fda046a23a215cc5e5a0687ddaf10b7c45a3ee8e10490361dc8d390cd00

.. code-block:: cpp

    /// `Hasher` calculates a fixed-output cryptographic digest incrementally.
    ///
    /// Select an explicit algorithm when a file format or protocol defines it, add the exact bytes in one or more calls,
    /// and call `finalize()` after the complete message has been processed. Text updates hash the exact UTF-8 bytes stored
    /// in the string.
    void fixedSha3() {
        auto hasher = el::Hasher{el::HashAlgorithm::Sha3_256};

        // Feed one logical observation to the hasher in two pieces.
        hasher.update("Site: canopée nord; espèce: toucan à bec rouge; "_el);
        hasher.update("humidité: 87 %"_el);
        const auto digest = hasher.finalize();

        el::io::printLine("Algorithm: "_el, hasher.algorithm().toString());
        el::io::printLine("Digest: "_el, digest);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Algorithm: sha3-256
    Digest: 6315745f4e5640e1df729e2954720f88a9dfedb52dd7123b8eed79f2fb732136

.. erbsland-demo-end::

Hasher Lifecycle
----------------

Calling ``finalize()`` a second time returns the cached digest.
Call :cpp:func:`Hasher::reset() <erbsland::cryptology::Hasher::reset>` before reusing the hasher for a new message with
the same algorithm.
Calling ``update()`` after finalization is a logic error.

A default-constructed ``Hasher`` is an invalid placeholder.
Use :cpp:func:`Hasher::isValid() <erbsland::cryptology::Hasher::isValid>` before accessing a hasher that may still be
empty.
Copies initially share their complete streaming state and detach before either copy changes, which allows applications
to reuse a common message prefix.
Binary callers pass an owning ``ByteBlock`` or a ``ConstByteSpan``.
For compatible standard storage, ``mem::toConstByteSpan()`` creates the call-scoped view without copying.

Hashing Text
------------

The ``String`` overload hashes the exact UTF-8 bytes stored in the string.
It does not normalize Unicode, add a byte-order mark, validate text, or convert the encoding.
Visually identical strings can therefore have different digests when their Unicode representations differ.

Define a canonical representation before hashing structured or user-provided text.
When a format requires UTF-16, UTF-32, a byte-order mark, or another specific representation, encode the string first
with :cpp:class:`StringEncoder <erbsland::text::StringEncoder>` and hash the resulting byte block.

Select an Algorithm Automatically
=================================

Use :cpp:func:`HashSelector::recommended() <erbsland::cryptology::HashSelector::recommended>` when your application
controls the stored format and wants to follow the library and application-wide policy.
The default requirements select an acceptable general-purpose algorithm.

:cpp:struct:`HashRequirements <erbsland::cryptology::HashRequirements>` can require an exact
:cpp:enum:`CryptographicStatus <erbsland::cryptology::CryptographicStatus>`, a minimum
:cpp:enum:`CryptographicSecurity <erbsland::cryptology::CryptographicSecurity>`, and a minimum
:cpp:enum:`HashThroughput <erbsland::cryptology::HashThroughput>`.
The throughput values compare implementations in this library; they are not universal performance measurements.

The following demo requests high security with at least medium relative throughput.

.. erbsland-demo::
    :source: cryptology/HashAlgorithms/SelectAlgorithm.cpp
    :exec: cryptology/hash_algorithms --demo SelectAlgorithm
    :source-sha256: bb9e3065dce6cc97f00ccdaf6d0a66310eccca0771e02046a61ad132957831e2

.. code-block:: cpp

    /// `HashSelector` selects a supported algorithm from application requirements and current policy.
    ///
    /// Use `recommended()` when your application controls the format and can follow current library policy. Persist the
    /// returned algorithm identifier with the digest because recommendations and metadata can change in later releases.
    void selectAlgorithm() {
        const auto requirements = el::HashRequirements{
            .requiredStatus = el::CryptographicStatus::Acceptable,
            .minimumSecurity = el::CryptographicSecurity::High,
            .minimumThroughput = el::HashThroughput::Medium,
        };
        const auto algorithm = el::HashSelector{requirements}.recommended();

        if (!algorithm.has_value()) {
            el::io::printLine("No supported hash algorithm satisfies the requirements."_el);
            return;
        }

        el::io::printLine("Selected algorithm: "_el, algorithm->toString());
        el::io::printLine("Digest bytes: "_el, algorithm->digestSize().toSizeT());
    }

.. erbsland-ansi::
    :escape-char: ␛

    Selected algorithm: sha3-384
    Digest bytes: 48

.. erbsland-demo-end::

The result is optional because no built-in algorithm may satisfy every requirement.
Use :cpp:func:`HashSelector::matching() <erbsland::cryptology::HashSelector::matching>` when the caller wants to inspect
every qualifying algorithm instead of accepting the single recommendation.

Algorithm selection status is library and application policy and can change as implementations, cryptographic guidance,
or administrative limits evolve.
Do not use :cpp:func:`HashAlgorithm::security() <erbsland::cryptology::HashAlgorithm::security>` alone as a safety
decision.
An algorithm can have a nominal output strength while its status disallows cryptographic use.
Use :cpp:func:`HashSelector::isSafe() <erbsland::cryptology::HashSelector::isSafe>` or require an acceptable status.

Store Hash Values
=================

:cpp:func:`Hasher::finalize() <erbsland::cryptology::Hasher::finalize>` returns a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>`.
Keep those bytes unchanged in a binary format.
The Erbsland Configuration Language also has a native byte value, written as hexadecimal data between angle brackets.

.. warning::

    Do not hash passwords with :cpp:class:`Hasher <erbsland::cryptology::Hasher>`.
    Use :cpp:class:`PasswordHasher <erbsland::cryptology::PasswordHasher>` as described in
    :doc:`storing_and_verifying_passwords`.
    Password hashing needs a random salt, deliberately expensive derivation, a canonical storage record, and migration
    handling that a general message digest does not provide.

Text-only formats need an explicit binary-to-text encoding.
Base16 is easy to inspect and compare but uses two characters for every byte.
Base64 is more compact.
Choose one canonical encoding for the format, document whether letter case and padding matter, and decode untrusted text
with a maximum output size.

This demo writes a SHA3-256 digest into a Base16 sidecar file and decodes it with the expected digest size as a hard
limit.

.. erbsland-demo::
    :source: cryptology/HashAlgorithms/StoreDigest.cpp
    :exec: cryptology/hash_algorithms --demo StoreDigest
    :source-sha256: 80e74be7bb81b1fab191079247e0f4bf0e00c670ea970d2bbf866481b0da8410

.. code-block:: cpp

    /// Store a binary digest in a text file using an explicit Base-N encoding.
    ///
    /// Base16 is easy to inspect and uses two characters per digest byte. Base64 is more compact when the surrounding
    /// format permits it. Decode untrusted text with the expected digest size as a hard output limit.
    void storeDigest() {
        auto hasher = el::Hasher{el::HashAlgorithm::Sha3_256};
        hasher.update("Inventaire: orchidée miniature, parcelle Émeraude-4"_el);
        const auto digest = hasher.finalize();

        const auto format = el::text::base_n::BaseNFormat::base16();
        const auto encoded = el::text::base_n::BaseNEncoder{digest, format}.toString();

        auto directoryOptions = el::PathTempDirectoryOptions{};
        directoryOptions.setPrefix("canopée-"_el).setSuffix("-hash-demo"_el);
        const auto directory =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(directoryOptions);
        const auto sidecarPath = directory->path() / "observation.sha3-256"_el;
        sidecarPath.content().writeTextOrThrow(encoded);

        const auto storedText = sidecarPath.content().readTextOrThrow(el::PathReadTextOptions{el::ByteLength{128U}});
        const auto decoded =
            el::text::base_n::BaseNDecoder{storedText, format}.toDataOrThrow(hasher.algorithm().digestSize());

        el::io::printLine("Stored characters: "_el, storedText.characterLength().toSizeT());
        el::io::printLine("Round trip matches: "_el, el::BooleanFormat::yesNo(), decoded == digest);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Stored characters: 64
    Round trip matches: yes

.. erbsland-demo-end::

Persist the Used Algorithm
==========================

Store the algorithm identifier beside every digest that must be interpreted later.
:cpp:func:`HashAlgorithm::toString() <erbsland::cryptology::HashAlgorithm::toString>` returns a stable lowercase
identifier such as ``sha3-256``.
Do not persist the raw enumeration value, the selection requirements, or an assumption about what ``recommended()`` will
return in a future release.

A structured record can keep the identifier and digest together:

.. code-block:: elcl

    [canopy_record]
    algorithm: "sha3-256"
    digest: <6315745f4e5640e1df729e2954720f88a9dfedb52dd7123b8eed79f2fb732136>

When reading the record, parse the identifier with
:cpp:func:`HashAlgorithm::fromString() <erbsland::cryptology::HashAlgorithm::fromString>`, apply current policy, and
check that the digest length equals
:cpp:func:`HashAlgorithm::digestSize() <erbsland::cryptology::HashAlgorithm::digestSize>`.
Reject unknown identifiers instead of silently substituting a default.

.. erbsland-demo::
    :source: cryptology/HashAlgorithms/PersistAlgorithm.cpp
    :exec: cryptology/hash_algorithms --demo PersistAlgorithm
    :source-sha256: d0405c70dc6830c16b7d8176fe8b3293434c6985ca29e252df5b0ebe7e72f53d

.. code-block:: cpp

    /// Persist the stable algorithm identifier beside every digest that must be interpreted later.
    ///
    /// Parse identifiers with `HashAlgorithm::fromString()`, apply current safety policy, and verify that the stored digest
    /// has the size required by the selected algorithm before using the record.
    void persistAlgorithm() {
        const auto algorithm = el::HashAlgorithm{el::HashAlgorithm::Sha3_256};
        auto hasher = el::Hasher{algorithm};
        hasher.update("Relevé: paresseux à trois doigts, hauteur 28 m"_el);
        const auto digest = hasher.finalize();

        const auto configuration = el::StringFormat{"[canopy_record]\n"
                                                    "algorithm: \"{}\"\n"
                                                    "digest: <{}>\n"_el}
                                       .build(algorithm.toString(), digest);
        const auto document = el::conf::Parser{}.parseTextOrThrow(configuration);

        const auto algorithmText = document->getTextOrThrow(el::String{"canopy_record.algorithm"_el});
        const auto storedAlgorithm = el::HashAlgorithm::fromString(algorithmText);
        if (!storedAlgorithm.has_value() || !el::HashSelector{}.isSafe(storedAlgorithm.value())) {
            throw el::RuntimeError{"The stored hash algorithm is unknown or no longer acceptable."_el};
        }

        const auto storedDigest = document->getBytesOrThrow(el::String{"canopy_record.digest"_el});
        if (storedDigest.length() != storedAlgorithm->digestSize()) {
            throw el::RuntimeError{"The digest size does not match the stored hash algorithm."_el};
        }

        el::io::printLine("Algorithm accepted: "_el, storedAlgorithm->toString());
        el::io::printLine("Digest size valid: "_el, el::BooleanFormat::yesNo(), storedDigest == digest);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Algorithm accepted: sha3-256
    Digest size valid: yes

.. erbsland-demo-end::

Defensive Hashing
=================

Hash code often processes data chosen by another system or an untrusted user.
Protect the complete workflow, not only the call to the hash function.

* Reject unknown and disallowed algorithms to prevent downgrade attacks.
* Define one canonical byte representation before hashing structured data.
  Otherwise, the producer and verifier can hash different encodings of the same logical value.
* Limit decoded digest text to the exact size expected for the selected algorithm.
* Stream large input in bounded blocks and enforce an application-level total size or deadline.
* Treat the expected digest as trusted metadata.
  A plain digest does not authenticate data when an attacker can replace both values.
* Do not build a keyed construction by concatenating a secret and a message.
  In particular, SHA-2 follows a Merkle–Damgård construction for which naive secret-prefix designs can permit
  length-extension attacks.
  Use a reviewed message-authentication construction.
* Do not store passwords with a fast general-purpose hash.
  Password storage requires a salted password-hashing or key-derivation function with a configurable work factor,
  preferably one designed to consume substantial memory.

The following demo opens a file once, hashes it in fixed-size chunks, enforces a total byte limit, and accepts only an
algorithm that passes the current safety policy.

.. erbsland-demo::
    :source: cryptology/HashAlgorithms/HashBoundedInput.cpp
    :exec: cryptology/hash_algorithms --demo HashBoundedInput
    :source-sha256: 2ab28dbe4b31d38bd6c9a11a35a8ea9db073bf87c08afcd16e2b132d3ae299d8

.. code-block:: cpp

    /// Hash a file through a bounded stream and current algorithm policy.
    ///
    /// Open the file once, process its bytes incrementally, and keep both the memory use and total accepted input bounded.
    /// Check `HashSelector::isSafe()` before processing data that names its own algorithm.
    void hashBoundedInput() {
        auto directoryOptions = el::PathTempDirectoryOptions{};
        directoryOptions.setPrefix("forêt-"_el).setSuffix("-hash-demo"_el);
        const auto directory =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(directoryOptions);
        const auto observationPath = directory->path() / "inventaire.txt"_el;
        observationPath.content().writeTextOrThrow(
            "Parcelle: Ambre-7\nStrate: canopée\nObservation: deux aras chloroptères\n"_el);

        const auto input = observationPath.content().openByteInputStream();
        const auto digest = hashBoundedStream(*input, el::HashAlgorithm::Sha3_256, 1024U);
        input->close();

        el::io::printLine("Digest bytes: "_el, digest.length().toSizeT());
        el::io::printLine(
            "MD5 rejected: "_el,
            el::BooleanFormat::yesNo(),
            !el::HashSelector{}.isSafe(el::HashAlgorithm::Md5));
    }

    /// Hash an untrusted stream without collecting it in memory or accepting unlimited input.
    auto hashBoundedStream(el::ByteInputStream &input, const el::HashAlgorithm algorithm, const std::size_t maximumBytes)
        -> el::ByteBlock {
        if (!el::HashSelector{}.isSafe(algorithm)) {
            throw el::RuntimeError{"The selected hash algorithm is not acceptable."_el};
        }

        auto hasher = el::Hasher{algorithm};
        auto buffer = std::array<el::Byte, 64U>{};
        auto totalBytes = std::size_t{0U};

        while (true) {
            const auto result = input.read(buffer);
            if (result.isFinished()) {
                break;
            }
            if (result.isTimeout()) {
                throw el::RuntimeError{"Reading the input for hashing timed out."_el};
            }

            const auto blockLength = result.data().toSizeT();
            if (blockLength > maximumBytes - std::min(totalBytes, maximumBytes)) {
                throw el::RuntimeError{"The input exceeds the permitted hashing limit."_el};
            }
            totalBytes += blockLength;
            hasher.update(std::span<const el::Byte>{buffer.data(), blockLength});
        }

        return hasher.finalize();
    }

.. erbsland-ansi::
    :escape-char: ␛

    Digest bytes: 32
    MD5 rejected: yes

.. erbsland-demo-end::
