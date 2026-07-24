..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Supported Hash Algorithms
    single: Cryptology
    single: SHA3-256
    single: SHA3-384
    single: SHA3-512
    single: SHA-256
    single: SHA-384
    single: SHA-512
    single: SHA-1
    single: MD5

.. _cryptology-supported-hash-algorithms:

*************************
Supported Hash Algorithms
*************************

This page describes every fixed-output hash algorithm supported by Erbsland Core.
You will learn the size and current security status of each algorithm, which algorithms are suitable for new formats,
and which exist only for legacy verification.

These are general message-digest algorithms.
None is suitable for storing passwords directly, including the SHA-2 and SHA-3 variants with large outputs.
Use :cpp:class:`PasswordHasher <erbsland::cryptology::PasswordHasher>` and see
:doc:`supported_password_hashing_algorithms` for the deliberately expensive algorithms provided for passwords.

The classifications follow the library's current policy and guidance from the `NIST hash-functions project
<https://csrc.nist.gov/Projects/hash-functions>`_.
NIST specifies SHA-2 in `FIPS 180-4 <https://doi.org/10.6028/NIST.FIPS.180-4>`_ and SHA-3 in `FIPS 202
<https://doi.org/10.6028/NIST.FIPS.202>`_.
Algorithm status can change when new attacks or guidance become available, so applications should apply current policy
when reading persisted algorithm identifiers.

Overview
========

Collision strength estimates the work needed to find any two messages with the same digest.
It is normally half the digest width for the acceptable algorithms listed below.
Preimage resistance is stronger, but collision resistance is often the limiting property for signatures and
content-integrity designs.

.. list-table::
    :header-rows: 1
    :widths: 16 14 18 16 36

    *   - Algorithm
        - Digest
        - Collision strength
        - Status
        - Recommended role
    *   - :ref:`sha3-256 <cryptology-hash-sha3-256>`
        - 32 bytes
        - 128 bits
        - Acceptable
        - General-purpose default
    *   - :ref:`sha3-384 <cryptology-hash-sha3-384>`
        - 48 bytes
        - 192 bits
        - Acceptable
        - Higher security margin
    *   - :ref:`sha3-512 <cryptology-hash-sha3-512>`
        - 64 bytes
        - 256 bits
        - Acceptable
        - Maximum SHA-3 margin
    *   - :ref:`sha-256 <cryptology-hash-sha2-256>`
        - 32 bytes
        - 128 bits
        - Acceptable
        - SHA-2 interoperability
    *   - :ref:`sha-384 <cryptology-hash-sha2-384>`
        - 48 bytes
        - 192 bits
        - Acceptable
        - SHA-2 interoperability with a higher margin
    *   - :ref:`sha-512 <cryptology-hash-sha2-512>`
        - 64 bytes
        - 256 bits
        - Acceptable
        - Maximum SHA-2 margin
    *   - :ref:`sha-1 <cryptology-hash-sha1>`
        - 20 bytes
        - Broken
        - Disallowed
        - Legacy verification only
    *   - :ref:`md5 <cryptology-hash-md5>`
        - 16 bytes
        - Broken
        - Disallowed
        - Legacy verification only

.. _cryptology-hash-sha3-256:

SHA3-256
========

SHA3-256 is the general-purpose SHA-3 variant with a 256-bit digest and an estimated 128-bit collision strength.
It is based on the Keccak sponge construction standardized in FIPS 202 and is the library's default algorithm.

Use SHA3-256 for new content identifiers, trusted integrity metadata, and higher-level cryptographic constructions that
require a 256-bit SHA-3 digest.
It offers the same nominal collision and preimage strengths as SHA-256 but uses a different internal construction.
Unlike a naive use of SHA-256's iterative construction, SHA3-256 is not subject to the classic length-extension pattern.
Use a standard authenticated construction whenever a key is involved, regardless of the underlying family.

.. _cryptology-hash-sha3-384:

SHA3-384
========

SHA3-384 produces a 384-bit digest with an estimated 192-bit collision strength.
It uses the same SHA-3 sponge design as SHA3-256 with a larger capacity and a lower input rate.

Use SHA3-384 when a format needs more than 128 bits of collision strength or must align with cryptographic components
that target a 192-bit security level.
Compared with SHA3-256, it provides a larger margin at the cost of a longer digest and lower relative throughput.
Compared with SHA-384, it has the same nominal strengths but a different construction and identifier.

.. _cryptology-hash-sha3-512:

SHA3-512
========

SHA3-512 produces a 512-bit digest with an estimated 256-bit collision strength.
It has the largest capacity and security margin among the supported SHA-3 variants.

Use SHA3-512 when a design explicitly requires a 512-bit SHA-3 digest or a 256-bit collision-strength target.
Its output is twice as large as SHA3-256 and the bundled implementation has lower relative throughput.
The extra width is not normally needed for general-purpose content hashing.
Compared with SHA-512, it provides the same nominal strengths through the SHA-3 sponge construction rather than the
SHA-2 compression-function construction.

.. _cryptology-hash-sha2-256:

SHA-256
=======

SHA-256 is the 256-bit member of the SHA-2 family standardized in FIPS 180-4. It has an estimated 128-bit collision
strength and remains acceptable for new cryptographic results.

Use SHA-256 when a protocol, file format, or external system requires SHA-2 interoperability.
It has the same digest size and nominal strengths as SHA3-256. SHA-256 processes 512-bit blocks with 32-bit operations,
while SHA3-256 uses a sponge construction.
SHA-2's iterative design also means that naive secret-prefix uses can permit length extension; use a standard
message-authentication construction instead.

.. _cryptology-hash-sha2-384:

SHA-384
=======

SHA-384 is a SHA-2 variant built from the SHA-512 design with distinct initial values and a 384-bit result.
It provides an estimated 192-bit collision strength.

Use SHA-384 for protocols and cryptographic suites that require SHA-2 with a 192-bit security target.
Compared with SHA-256, it uses 64-bit words, a larger block size, and a longer digest.
Compared with SHA3-384, it provides the same nominal strengths but belongs to the older SHA-2 construction and may be
required for interoperability.

.. _cryptology-hash-sha2-512:

SHA-512
=======

SHA-512 is the full-width 512-bit SHA-2 variant.
It provides an estimated 256-bit collision strength and uses 1024-bit blocks with 64-bit operations.

Use SHA-512 when a protocol explicitly requires it or when a SHA-2 design needs a 256-bit collision-strength target.
Its larger digest and lower bundled throughput make it unnecessary for most general-purpose hashes.
Compared with SHA-384, it retains the complete internal result.
Compared with SHA3-512, it has the same nominal strengths but different construction, interoperability, and performance
characteristics.

.. _cryptology-hash-sha1:

SHA-1
=====

SHA-1 produces a 160-bit digest, but practical collision attacks have invalidated its collision resistance.
NIST has deprecated SHA-1 and is `transitioning away from it for all cryptographic protection
<https://www.nist.gov/news-events/news/2022/12/nist-transitioning-away-sha-1-all-applications>`_.
The library therefore classifies SHA-1 as disallowed.

Use SHA-1 only when an existing format requires reproducing or checking a historical digest.
Do not use it for new signatures, certificates, content-integrity records, or security protocols.
When migrating legacy data, verify the old value under an explicit legacy policy and write a new digest with an
acceptable algorithm.

.. _cryptology-hash-md5:

MD5
===

MD5 produces a 128-bit digest and has practical collision attacks.
`RFC 6151 <https://www.rfc-editor.org/rfc/rfc6151>`_ states that MD5 is no longer acceptable where collision resistance
is required, and `RFC 9155 <https://www.rfc-editor.org/rfc/rfc9155>`_ deprecates MD5 and SHA-1 signature hashes in TLS
1.2. The library classifies MD5 as disallowed.

Use MD5 only to reproduce or check a value required by a legacy format.
It must not protect new data against an active attacker and must not be used for signatures, certificates, password
storage, or new integrity metadata.
Even when the application needs only a non-security checksum, prefer an algorithm or checksum selected explicitly for
that purpose instead of carrying MD5 into a new format.
