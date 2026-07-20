.. index::
    single: Cryptology
    single: Hashing
    single: SHA-3
    single: Hasher
    single: Hash Algorithm Selection

.. _cryptology-hashing:

*********************
Cryptographic Hashing
*********************

The cryptology domain provides fixed-output hash algorithms and a streaming
:cpp:class:`Hasher <erbsland::cryptology::Hasher>`.
This page explains how to select an algorithm, hash binary or text data, and store enough information to interpret a
digest later.

Hashing a Stream
================

Construct a hasher with a :cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>`, add data in one or more
calls, and finish with :cpp:func:`Hasher::finalize() <erbsland::cryptology::Hasher::finalize>`.
Calling ``finalize()`` again returns the cached digest.
Call :cpp:func:`Hasher::reset() <erbsland::cryptology::Hasher::reset>` before reusing the object for another stream.

.. code-block:: cpp

    auto hasher = el::Hasher{el::HashAlgorithm::Sha3_256};
    hasher.update(firstBlock);
    hasher.update(secondBlock);
    const auto digest = hasher.finalize();

Updating a finalized hasher is a logic error.
A default-constructed hasher is an invalid placeholder; test it with
:cpp:func:`Hasher::isValid() <erbsland::cryptology::Hasher::isValid>` before use.

Hasher copies are inexpensive.
They initially share the complete streaming state and detach when either copy is reset, updated, or finalized.
This lets you hash a shared prefix and then continue with independent suffixes.

Hashing Text
============

The :cpp:func:`Hasher::update() <erbsland::cryptology::Hasher::update>` text overload hashes the exact bytes already
stored in an :cpp:type:`String <erbsland::text::String>`.
It does not validate or normalize text, add a byte order mark, or perform an encoding conversion.
As ``String`` stores UTF-8, the resulting digest represents those exact UTF-8 code units.

.. code-block:: cpp

    auto hasher = el::Hasher{el::HashAlgorithm::Sha3_256};
    hasher.update(el::String{"Grüezi"_el});
    const auto digest = hasher.finalize();

When a digest must represent UTF-16, UTF-32, a byte order mark, or another explicit representation, encode the string
first with :cpp:class:`StringEncoder <erbsland::text::StringEncoder>` and hash the resulting byte block.

Selecting an Algorithm
======================

Each :cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>` has status, security, and relative-throughput
metadata.
The metadata is library policy and may change in later releases as cryptographic guidance and implementations evolve.
The initial classifications follow `NIST transition guidance <https://csrc.nist.gov/pubs/sp/800/131/a/r2/final>`_ and
NIST's published `hash-function security strengths <https://csrc.nist.gov/Projects/hash-functions>`_.

:cpp:struct:`HashRequirements <erbsland::cryptology::HashRequirements>` makes selection constraints explicit.
:cpp:func:`HashAlgorithm::matching() <erbsland::cryptology::HashAlgorithm::matching>` returns every match, while
:cpp:func:`HashAlgorithm::recommended() <erbsland::cryptology::HashAlgorithm::recommended>` chooses the fastest
qualifying algorithm, then prefers the higher security level.

.. code-block:: cpp

    const auto requirements = el::HashRequirements{
        .minimumSecurity = el::CryptographicSecurity::High,
        .minimumThroughput = el::HashThroughput::Low,
    };
    const auto algorithm = el::HashAlgorithm::recommended(requirements);

An empty result means that no built-in algorithm satisfies every requirement.

Storing Digests
===============

Store the algorithm identifier next to every digest that must be interpreted later.
:cpp:func:`HashAlgorithm::toString() <erbsland::cryptology::HashAlgorithm::toString>` returns stable lowercase names
such as ``sha3-256``.
Do not store only the requirements or assume that ``recommended()`` will select the same algorithm in every future
library release.

Cryptographic hashes are not password-storage algorithms.
Password storage needs a dedicated salted password hashing or key-derivation scheme with a configurable work factor.
A plain SHA-3 digest does not provide these properties.
