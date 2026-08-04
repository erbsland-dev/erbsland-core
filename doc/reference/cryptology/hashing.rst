..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Cryptology
    single: HashAlgorithm
    single: Hasher

*********************
Cryptographic Hashing
*********************

:cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>` describes a fixed-output hash and its current
intrinsic metadata.
:cpp:class:`Hasher <erbsland::cryptology::Hasher>` calculates a digest incrementally with copy-on-write state.

Algorithm Selection
===================

:cpp:struct:`HashRequirements <erbsland::cryptology::HashRequirements>` combines status, security, and throughput
requirements for selecting a :cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>`.
:cpp:class:`HashSelector <erbsland::cryptology::HashSelector>` applies these requirements to one coherent snapshot of
the application-wide cryptology policy.
Its ``status()``, ``isSafe()``, ``allAccepted()``, ``matching()``, and ``recommended()`` operations always observe the
current policy.
Configured status limits are ceilings: they can downgrade an algorithm, but never promote a library status.
Explicit :cpp:class:`Hasher <erbsland::cryptology::Hasher>` construction remains available for protocols and migration
work even when selection policy disallows the algorithm.
See :doc:`/topics/cryptology/using_hash_algorithms` for selection and persistence workflows and
:doc:`/topics/cryptology/supported_hash_algorithms` for the algorithm catalog and current safety guidance.

Hashing Data
============

:cpp:class:`Hasher <erbsland::cryptology::Hasher>` accepts text or byte data in one or more updates and returns a
:cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` digest.
Binary updates use :cpp:type:`ConstByteSpan <erbsland::mem::ConstByteSpan>` or an owning byte block.
Standard byte, unsigned-byte, and character spans can cross that boundary without copying through
``mem::toConstByteSpan()``; no public ``std::span<const std::byte>`` overload is provided.
``secureErase()`` wipes uniquely owned message-dependent state and restores a fresh state for the same algorithm.
For shared copy-on-write state, it installs a fresh worker without copying secret state and leaves copied hashers
unchanged.
An invalid placeholder treats secure erasure as a no-op.
See :doc:`/topics/cryptology/using_hash_algorithms` for lifecycle, representation, storage, and defensive-input
guidance.

Interface
=========

.. doxygenenum:: erbsland::cryptology::CryptographicSecurity
.. doxygenenum:: erbsland::cryptology::CryptographicStatus
.. doxygenclass:: erbsland::cryptology::HashAlgorithm
    :members:
.. doxygenclass:: erbsland::cryptology::Hasher
    :members:
.. doxygenstruct:: erbsland::cryptology::HashRequirements
    :members:
.. doxygenclass:: erbsland::cryptology::HashSelector
    :members:
.. doxygenenum:: erbsland::cryptology::HashThroughput
