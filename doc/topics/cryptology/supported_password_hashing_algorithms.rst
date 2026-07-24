..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Supported Password Hashing Algorithms
    single: Password Hashing Algorithm
    single: Argon2id
    single: scrypt
    single: Pepper
    single: HMAC-SHA-256
    single: Password Hash Migration
    single: PBKDF2
    single: bcrypt

*************************************
Supported Password Hashing Algorithms
*************************************

This page describes the password-hashing algorithms and reviewed policies in Erbsland Core.
You will learn how Argon2id and scrypt slow offline guesses, how the separate HMAC pepper layer protects their output,
and why other hashing APIs are excluded from password storage.

Use :cpp:class:`PasswordHashPolicy <erbsland::cryptology::PasswordHashPolicy>` presets instead of selecting numeric
costs yourself.
Recommendations can change as hardware and cryptographic guidance evolve.
Every stored :cpp:class:`PasswordHash <erbsland::cryptology::PasswordHash>` records the exact algorithm and costs, so
successful verification can migrate older records.

Algorithm Overview
==================

.. list-table::
    :header-rows: 1
    :widths: 18 26 28 28

    *   - Algorithm
        - Reviewed policy
        - Construction
        - Intended use
    *   - Argon2id
        - 64 MiB, 3 passes, 4 lanes
        - Memory-hard hybrid addressing
        - Recommended default
    *   - Argon2id low-memory
        - 19 MiB, 2 passes, 1 lane
        - Same construction with a smaller memory budget
        - Constrained trusted environments
    *   - scrypt
        - ``N=2^17``, ``r=8``, ``p=1``
        - PBKDF2 and Salsa20/8 BlockMix ROMix
        - Compatibility and migration

All presets use a 16-byte random salt and produce a 32-byte raw output.
The normal keyed mode then protects that output with HMAC-SHA-256 before storage.

Argon2id
========

Argon2id version 1.3 combines two addressing strategies.
The first half of its first pass uses data-independent memory references to limit password-dependent cache-timing
leakage.
The remaining work uses data-dependent references to strengthen resistance to time-memory tradeoffs.
Its large memory matrix makes parallel password guessing expensive on CPUs, GPUs, and specialized hardware.

The implementation follows `RFC 9106 <https://www.rfc-editor.org/rfc/rfc9106.html>`_ and includes its BLAKE2b-based
variable-length hash and 1024-byte compression function directly in the library.
The code keeps the memory matrix and intermediate values in sensitive byte storage.

Recommended Policy
------------------

:cpp:func:`PasswordHashPolicy::recommended() <erbsland::cryptology::PasswordHashPolicy::recommended>` uses 64 MiB,
three passes, and four lanes.
This is the normal choice for interactive authentication when the service can reserve that memory for each concurrent
verification.

:cpp:func:`PasswordHashPolicy::lowMemory() <erbsland::cryptology::PasswordHashPolicy::lowMemory>` uses 19 MiB, two
passes, and one lane.
Use it only when the recommended policy cannot fit a measured memory or concurrency budget.
Reducing memory makes large-scale guessing less expensive, so prefer admission control and bounded authentication
concurrency over lowering the policy automatically under load.

scrypt
======

scrypt expands the password and salt with PBKDF2-HMAC-SHA-256, repeatedly mixes a large memory region with Salsa20/8
BlockMix and ROMix, then applies PBKDF2 again.
The memory dependency raises the cost of evaluating many guesses in parallel.

The implementation follows `RFC 7914 <https://www.rfc-editor.org/rfc/rfc7914.html>`_.
:cpp:func:`PasswordHashPolicy::scrypt() <erbsland::cryptology::PasswordHashPolicy::scrypt>` uses ``N=2^17``, ``r=8``,
and ``p=1``.
Argon2id remains the preferred default; scrypt provides a strong established alternative for migration and
interoperability requirements.

The Pepper Layer
================

For keyed records, the stored verifier is:

.. code-block:: text

    HMAC-SHA-256(application-key, canonical-header-through-salt || NUL || raw-password-hash)

The application key is not passed into the public salt field and never enters the stored record.
The raw Argon2id or scrypt output is also not stored.
Binding the canonical header authenticates the algorithm, costs, pepper mode, optional key identifier, and salt along
with the derived password value.

This layer follows the additional keyed-iteration guidance in `NIST SP 800-63B-4
<https://pages.nist.gov/800-63-4/sp800-63b.html>`_.
It is an additional control, not a substitute for a memory-hard derivation, unique salts, database access controls, or
rate limiting.

Storage Format
==============

The serialized value is a strict, versioned, comma-separated record such as:

.. code-block:: text

    f:el-password-hash,v:1,a:argon2id,av:19,m:65536,t:3,p:4,x:hmac-sha256,s:<salt>,d:<verifier>

Identified peppers add an ``i:`` field.
Explicitly unkeyed records use ``x:none``.
Salt and verifier fields use unpadded Base64url.

The parser requires fixed field order and canonical numeric and Base64url encodings.
It rejects unknown fields and versions, records longer than 512 bytes, arithmetic overflow, and costs beyond hard
resource limits before allocating an algorithm workspace.
Applications should still treat the complete record as opaque and use
:cpp:func:`PasswordHash::fromString() <erbsland::cryptology::PasswordHash::fromString>` and
:cpp:func:`PasswordHash::toString() <erbsland::cryptology::PasswordHash::toString>`.

Automatic Migration
===================

The active :cpp:class:`PasswordHasher <erbsland::cryptology::PasswordHasher>` policy defines what new records should
look like.
After a successful verification it returns a replacement whenever any of these properties differs:

* storage format version,
* password-hashing algorithm,
* algorithm costs,
* keyed or unkeyed mode,
* active pepper identifier or key material.

Persisting this replacement gradually upgrades active accounts.
Keep old identified peppers available as fallbacks until their records have migrated.
See :doc:`storing_and_verifying_passwords` for the login and rotation workflows.

Unsafe Custom Costs
===================

:cpp:class:`UnsafeCustomPasswordHashParameters
<erbsland::cryptology::unsafe::UnsafeCustomPasswordHashParameters>` permits checked custom Argon2id or scrypt costs.
It lives in ``cryptology::unsafe`` and is not imported into the flattened ``erbsland`` namespace.
The constructor validates arithmetic and hard ceilings of 1 GiB memory, ten Argon2 passes, sixteen lanes, and equivalent
scrypt work.

The hard limits prevent obviously dangerous stored parameters; they do not make a cheap custom policy secure.
Use custom costs only after measuring production hardware, peak authentication concurrency, latency targets, and
denial-of-service exposure.
Keep the reviewed presets unless a documented deployment constraint requires a different choice.

Why Other Algorithms Are Excluded
=================================

General SHA and Hasher
----------------------

SHA-2 and SHA-3 are intentionally fast general message digests.
That is useful for file integrity and protocol constructions but also lets an attacker test password guesses quickly.
A salt does not make a fast digest deliberately expensive.
The public password API therefore never accepts a :cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>`.

PBKDF2
------

PBKDF2-HMAC-SHA-256 is implemented only as an internal part of scrypt.
Its cost is primarily CPU work and does not provide the memory-hard behavior selected for this API.
The public password API does not create standalone PBKDF2 records.

bcrypt
------

bcrypt remains important in existing systems, but its historical password-length and input-handling constraints make
transparent migration easy to get wrong.
Erbsland Core does not create or parse bcrypt records.
Applications migrating bcrypt data should verify it through a dedicated compatibility component, then write an Argon2id
:cpp:class:`PasswordHash <erbsland::cryptology::PasswordHash>` after the next successful login.
