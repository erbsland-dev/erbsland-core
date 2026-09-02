..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Cryptographic Operations; Reference
    single: Cryptology
    single: HashAlgorithm
    single: Hasher
    single: HMAC
    single: HKDF
    single: Message Authentication
    single: Key Derivation
    single: Symmetric Encryption
    single: Authenticated Encryption
    single: AES-GCM
    single: ChaCha20-Poly1305
    single: Fast File Encryption Compatibility
    single: Application

************************
Cryptographic Operations
************************

Application-Wide Policy
=======================

:cpp:func:`Application::cryptologyConfiguration() <erbsland::core::Application::cryptologyConfiguration>` lazily
constructs the process-wide :cpp:class:`CryptologyConfiguration <erbsland::cryptology::CryptologyConfiguration>`.
Its storage belongs to the shared application data, so applications connected with ``Application::linkWith()`` observe
the same configuration across module and DLL boundaries.
Configuration access and changes are thread-safe, and list or recommendation operations use one coherent snapshot.

``setHardwareAccelerationEnabled(false)`` is an operational escape hatch for deployments where a compiled accelerated
backend fails on the installed platform.
Configure it during early startup, before constructing cryptographic workers.
The flag permits automatic backend selection when enabled; it does not claim that hardware support is present.
Changes apply only to subsequently constructed workers.

``setMaximumStatus()`` sets a downgrade-only ceiling for an individual hash algorithm or symmetric encryption type.
The effective selector status is the less permissive of library policy and this ceiling, so configuration cannot promote
a ``Legacy`` or ``Disallowed`` algorithm.
``clearMaximumStatus()`` removes one ceiling and ``reset()`` restores all defaults.
Policy changes affect selector results, not explicit hashing, encryption, or decryption required for existing data.

Cryptographic Hashing
=====================

:cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>` describes a fixed-output hash and its current
intrinsic metadata.
:cpp:class:`Hasher <erbsland::cryptology::Hasher>` calculates a digest incrementally with copy-on-write state.

Algorithm Selection
-------------------

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
------------

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

Message Authentication and Key Derivation
=========================================

:cpp:class:`Hmac <erbsland::cryptology::Hmac>` implements streaming HMAC with SHA-256 or SHA-384.
:cpp:class:`Hkdf <erbsland::cryptology::Hkdf>` implements the generic RFC 5869 extract and expand operations for the
same hash algorithms.

Message Authentication
----------------------

An HMAC state retains secret key material and is therefore move-only.
It accepts segmented bytes or the exact stored UTF-8 bytes of a string, caches the complete authenticator after
finalization, and can reset to authenticate another message with the same key.
``verify()`` accepts only a complete hash-sized authenticator; equal-length values are compared without
content-dependent short-circuiting.
``secureErase()`` immediately erases retained keyed and message-dependent state and leaves an invalid placeholder.

Key Derivation
--------------

An HKDF value stores only its selected hash algorithm.
``extract()`` returns a digest-sized pseudorandom key and ``expand()`` returns the requested output keying material.
Every non-empty result uses sensitive storage that is securely erased when its final shared reference is released.
An empty salt follows RFC 5869 and acts as a digest-sized all-zero salt.
The shared-secret overload accepts a
:cpp:class:`KeyAgreementSharedSecret <erbsland::cryptology::KeyAgreementSharedSecret>` and performs extraction through
scoped plaintext access.
Expansion requires a pseudorandom key at least as long as the digest and accepts at most 255 digest blocks: 8,160 bytes
for SHA-256 or 12,240 bytes for SHA-384.

Symmetric Encryption
====================

``SymmetricEncryptionType`` identifies a complete symmetric encryption construction and describes its key, nonce,
initialization-vector, tag, and output-length requirements.
``SymmetricEncryptor`` and ``SymmetricDecryptor`` are move-only facades for incremental processing.

Algorithm Selection
-------------------

The accepted constructions for new data are ``aes-256-gcm``, ``chacha20-poly1305``, and ``aes-128-gcm``.
The default recommendation is ``aes-256-gcm``.
Use :cpp:class:`SymmetricEncryptionSelector <erbsland::cryptology::SymmetricEncryptionSelector>` to enumerate or
recommend constructions using the current application-wide policy.
The :cpp:struct:`SymmetricEncryptionRequirements <erbsland::cryptology::SymmetricEncryptionRequirements>` type can
filter choices by status, minimum security, cipher family, and authenticated-encryption support.
The selector preserves the preference order AES-256-GCM, ChaCha20-Poly1305, AES-128-GCM, then the legacy constructions.
Configured status limits are downgrade-only ceilings and do not disable explicit construction for existing protocols.

Call :cpp:func:`SymmetricEncryptionType::maximumEncryptedLength()
<erbsland::cryptology::SymmetricEncryptionType::maximumEncryptedLength>` before processing to reserve a predictable
output container size.
The result includes CBC alignment and padding, but excludes a separately transported AEAD tag.

Authenticated Encryption
------------------------

Add all authenticated associated data before the first non-empty payload part.
Encryption returns the authentication tag separately after successful finalization.
AEAD decryption can return plaintext incrementally, but that plaintext is unauthenticated and unsafe to consume, parse,
display, or otherwise act upon until ``finalize(tag)`` succeeds.
Discard every previously returned plaintext byte if authentication fails.

AES-GCM uses a fixed 96-bit nonce and a full 128-bit authentication tag.
A nonce must be unique for every message encrypted under the same key; nonce reuse destroys GCM's confidentiality and
authentication guarantees.
A single message accepts at most :math:`2^{36} - 32` payload bytes and :math:`2^{61} - 1` byte-aligned
authenticated-data bytes, as constrained by NIST SP 800-38D.

ChaCha20-Poly1305 implements the IETF construction from RFC 8439 with a 256-bit key, fixed 96-bit nonce, and full
128-bit tag.
It uses counter zero only to derive the Poly1305 one-time key and counters one through :math:`2^{32} - 1` for payload.
Consequently, one message accepts at most :math:`(2^{32} - 1) \times 64` payload bytes.
As with AES-GCM, a nonce must never be reused with the same key.

Legacy Fast File Encryption Compatibility
-----------------------------------------

The two AES-256-CBC constructions exist only for compatibility with Fast File Encryption data.
They are marked ``Legacy``, provide no authentication or integrity protection, and must not be used for new formats or
projects.
They deliberately remain separate types because their final-block behavior is incompatible.
Their 16-byte IV must be unpredictable and unique for encryption under a given key.
An IV is not secret, but changing it changes the first plaintext block during decryption.
These constructions therefore require an independently authenticated container when integrity matters.

Random-Fill Padding
~~~~~~~~~~~~~~~~~~~

``aes-256-cbc-random-fill`` implements the known-size Fast File Encryption mode.
If the plaintext length is not a multiple of the 16-byte AES block size, encryption appends unpredictable random bytes
until the final block is full.
If the plaintext is already block-aligned, encryption appends no bytes; empty plaintext therefore remains empty.
This is a non-standard, non-reversible fill scheme: ciphertext contains no indication of how many random bytes were
added.
Decryption consequently retains the random fill, and the container format must store the exact original plaintext length
separately so the caller can crop the decrypted result.

ISO/IEC 9797-1 Method 2 Padding
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``aes-256-cbc-iso9797-method2`` implements the chunked-stream Fast File Encryption mode.
Encryption always appends one ``0x80`` marker byte followed by enough zero bytes to reach the next 16-byte boundary.
An aligned or empty plaintext therefore gains one complete block.
Decryption validates and removes this marker-and-zero suffix, making the padding reversible without an externally stored
plaintext length.

Sensitive State and Erasure
---------------------------

``SymmetricKey`` does not expose its bytes through the public API.
Keys, tags, nonces, initialization vectors, and decrypted output use sensitive-marked storage.
Call ``secureErase()`` when encryption or decryption work ends to erase retained key material and backend state
immediately.
The call leaves the facade in the same empty state as default construction; create or assign a new encryptor or
decryptor before another operation.

Backend Availability
--------------------

AES uses automatic, independent backend selection for block encryption and GHASH multiplication.
The portable fallback uses fixed-iteration GF(2\ :sup:`8` ) arithmetic for the AES S-box and fixed-iteration GF(2\
:sup:`128` ) multiplication for GHASH, without secret-indexed tables.
On supported x86-64 processors the implementation uses AES-NI and PCLMULQDQ; on supported ARM64 processors it uses the
Arm AES and PMULL instructions.
If only one acceleration feature is available, the other operation remains on its portable implementation.

ChaCha20 and Poly1305 are independently dispatched.
The portable ChaCha20 backend is scalar, and portable Poly1305 uses five 26-bit limbs with 64-bit products and no
``__int128`` dependency.
x86-64 uses SSE2 and ARM64 uses NEON for four-block batches, with scalar handling for short inputs and tails.

Architecture-specific backends are permission-based rather than a claim that every compiled instruction will work in the
deployed environment.
Set ``application().cryptologyConfiguration().setHardwareAccelerationEnabled(false)`` during early application startup
to force portable AES, GHASH, ChaCha20, and Poly1305 backends.
The change affects workers constructed afterwards; existing workers retain the backend with which they were created.

The implementation is designed to avoid secret-indexed memory access and data-dependent branches in its portable
cryptographic primitives.
This is a side-channel design goal, not a formal guarantee that an entire application, compiler, operating system, or
hardware platform behaves in constant time.

The implemented constructions follow FIPS 197 for AES, NIST SP 800-38A for CBC, NIST SP 800-38D for GCM, and RFC 8439
for ChaCha20-Poly1305.

ChaCha20-Poly1305 Specification Mapping
---------------------------------------

The implementation deliberately keeps the RFC stages visible for security review:

* RFC 8439 Section 2.1 maps to ``ChaCha20Operations`` quarter-round code and the
  ``ChaCha20PrimitiveTest`` known-answer test.
* Sections 2.3 and 2.4 map to scalar and four-block ``ChaCha20Backend`` implementations, verified by block vectors and
  ``ChaCha20BackendFullTest`` differential tests.
* Section 2.5 maps to ``PortablePoly1305`` clamping, accumulation, reduction, and tag generation, with architecture
  kernels checked against it.
* Section 2.6 maps to the counter-zero one-time-key step in ``ChaCha20Poly1305State`` and its RFC vector test.
* Section 2.8 maps to shared AAD framing, padding, length serialization, counter advancement, and encryptor/decryptor
  workers, tested by RFC vectors, boundary tests, tamper tests, and the 325 pinned Wycheproof cases.

Specifications
--------------

* `FIPS 197: Advanced Encryption Standard (AES) <https://csrc.nist.gov/pubs/fips/197/final>`_
* `NIST SP 800-38A: Recommendation for Block Cipher Modes of Operation
  <https://csrc.nist.gov/pubs/sp/800/38/a/final>`_
* `NIST SP 800-38D: Recommendation for Galois/Counter Mode (GCM)
  <https://csrc.nist.gov/pubs/sp/800/38/d/final>`_
* `RFC 8439: ChaCha20 and Poly1305 for IETF Protocols <https://www.rfc-editor.org/rfc/rfc8439.html>`_

Interface
=========

.. doxygenclass:: erbsland::cryptology::CryptologyConfiguration
    :members:
.. doxygenenum:: erbsland::cryptology::CryptographicSecurity
.. doxygenenum:: erbsland::cryptology::CryptographicStatus
.. doxygenclass:: erbsland::cryptology::CryptologyError
    :members:
.. doxygenclass:: erbsland::cryptology::HashAlgorithm
    :members:
.. doxygenclass:: erbsland::cryptology::Hasher
    :members:
.. doxygenstruct:: erbsland::cryptology::HashRequirements
    :members:
.. doxygenclass:: erbsland::cryptology::HashSelector
    :members:
.. doxygenenum:: erbsland::cryptology::HashThroughput
.. doxygenclass:: erbsland::cryptology::Hkdf
    :members:
.. doxygenclass:: erbsland::cryptology::Hmac
    :members:
.. doxygenclass:: erbsland::cryptology::CryptographicDataBlock
    :members:
.. doxygenenum:: erbsland::cryptology::SymmetricCipher
.. doxygenclass:: erbsland::cryptology::SymmetricDecryptor
    :members:
.. doxygenstruct:: erbsland::cryptology::SymmetricEncryptionRequirements
    :members:
.. doxygenclass:: erbsland::cryptology::SymmetricEncryptionSelector
    :members:
.. doxygenclass:: erbsland::cryptology::SymmetricEncryptionType
    :members:
.. doxygenclass:: erbsland::cryptology::SymmetricEncryptor
    :members:
.. doxygenclass:: erbsland::cryptology::SymmetricIv
    :members:
.. doxygenclass:: erbsland::cryptology::SymmetricKey
    :members:
.. doxygenclass:: erbsland::cryptology::SymmetricNonce
    :members:
.. doxygenclass:: erbsland::cryptology::SymmetricTag
    :members:
