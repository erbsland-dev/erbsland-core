..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: TLS; Reference
    single: TLS Configuration; Reference
    single: TLS Record Protection; Reference
    single: Application
    single: Signing Keys
    single: PKCS#8
    single: TLS Server Identity
    single: TLS Record Protection
    single: TLS 1.3

***********
TLS Support
***********

Configuration
=============

``setTlsConfiguration(label, configuration)`` stores one immutable copy of a ``TlsConfiguration`` under an
application-wide label.
Connections resolve and retain that copy when ``connect()`` starts, so replacement or clearing affects only later
connections.
``reset()`` and ``clearTlsConfigurations()`` remove every registered entry.

Resolution first tries the exact label, then removes the final slash-delimited segment until it reaches the empty
global-default label.
For example, ``http/client/internal`` tries ``http/client/internal``, ``http/client``, ``http``, and ``""``.
One complete entry is selected; fields are never merged between a child and its parent.
An unresolved label or a selected entry without the policy required by a connection role fails synchronously before the
one-shot connection is consumed.

Labels contain at most 16 non-empty slash-separated segments and 255 bytes.
Segments use lowercase ASCII letters, digits, and interior hyphens.
The registry contains at most 256 exact entries.
``hasTlsConfiguration()`` tests only the exact label, while ``resolveTlsConfiguration()`` returns the requested label,
matched label, and immutable entry as one coherent result.

``TlsConfiguration`` stores an optional explicit-anchor
:cpp:class:`X509ServerCertificatePolicy <erbsland::cryptology::X509ServerCertificatePolicy>` and an optional immutable
shared :cpp:class:`TlsServerIdentity <erbsland::cryptology::TlsServerIdentity>`.
Copies and resolved snapshots share the identity without copying its move-only private key.
Protected client identities remain deferred.

:cpp:class:`TlsConfigurationParser <erbsland::cryptology::TlsConfigurationParser>` turns one ELCL section-list entry
into a labeled :cpp:class:`TlsConfigurationEntry <erbsland::cryptology::TlsConfigurationEntry>` without accessing the
application singleton.
See :doc:`/topics/cryptology/configuring_tls` for the configuration schema, relative-file behavior, and registration
workflow.


Erbsland Core does not automatically read environment variables or command-line options for this interface.
The application decides how enterprise configuration is authenticated, parsed, and applied.

Server Identities
=================

:cpp:class:`TlsServerIdentity <erbsland::cryptology::TlsServerIdentity>` combines a leaf-first ordered certificate
chain with its move-only signing key.
Construction rejects empty values, a leaf/key mismatch, or a key that supports no TLS 1.3 ``CertificateVerify`` scheme.
Ed25519 points compare exactly, P-256/P-384 points compare after affine normalization so compressed certificate keys are
accepted, and RSA matching includes modulus, exponent, key family, and PSS restrictions.

``selectSignatureScheme()`` examines ClientHello schemes in peer order and returns the first compatible value.
:cpp:class:`TlsConfiguration <erbsland::cryptology::TlsConfiguration>` stores an optional identity through immutable
shared ownership.
Configuration copies and application snapshots therefore remain copyable without copying private key material, and an
existing snapshot retains its identity after a later configuration is replaced or cleared.

Hardware-backed keys, multiple or SNI-selected identities, and the TLS server protocol are outside this interface.

TLS 1.3 Record Protection
=========================

The record-protection API implements the authenticated TLS 1.3 record transformations from RFC 8446. A
``TlsRecordEncryptor`` and ``TlsRecordDecryptor`` each retain one independent traffic-secret generation and sequence
number.
They operate on complete records so the exact wire header used as authenticated data cannot diverge from the header
returned to or received from the network layer.

Cipher Suites and Traffic Secrets
---------------------------------

``TlsCipherSuite`` represents exactly ``TLS_AES_128_GCM_SHA256``, ``TLS_AES_256_GCM_SHA384``, and
``TLS_CHACHA20_POLY1305_SHA256``.
It maps each RFC 8446 wire value to its registry name, HKDF hash, and symmetric construction.
All supported constructions use a 96-bit nonce and a 128-bit authentication tag.

Create a move-only ``TlsTrafficSecret`` from exact SHA-256- or SHA-384-sized bytes.
The consuming factory erases its input allocation after copying the secret into ``ProtectedByteBlock``.
There is intentionally no plaintext accessor.
Record state resolves the secret only inside an erased callback while deriving a key, IV, or next-generation secret.

One Direction per Object
------------------------

Construct a separate ``TlsRecordEncryptor`` and ``TlsRecordDecryptor`` for each protocol direction.
``protect()`` returns one complete header-plus-ciphertext record.
``unprotect()`` accepts exactly one complete record and returns a move-only ``TlsRecordPlaintext`` only after
authentication and inner-plaintext validation succeed.

Callers choose the inner content type and an explicit padding length.
This API deliberately defines no automatic padding policy.
Empty application data is supported; empty Alert and Handshake content is rejected.
Stream buffering, alert serialization, the ``record_size_limit`` extension, automatic padding strategies, and other
optional record extensions remain in the network protocol layer.

Failures and Terminal State
---------------------------

``TlsRecordErrorCategory`` distinguishes decode errors, unexpected messages, authentication failure, record overflow,
and exhausted key usage.
This lets a network implementation select an alert without parsing diagnostic text.

A local ``protect()`` parameter error occurs before key or sequence use and leaves the encryptor usable.
Any failure after protection begins erases the sending generation so an uncertain nonce cannot be reused.
Every peer-record rejection or backend failure permanently erases the decryptor.
Authentication failure therefore exposes no plaintext and cannot be followed by another attempt with the same receiving
object.

Bounds and Key Usage
--------------------

The implementation limits ``TLSInnerPlaintext`` to :math:`2^{14} + 1` bytes and rejects an outer ``TLSCiphertext``
length above :math:`2^{14} + 256` bytes.
Supported records have the tighter construction bound of inner length plus the fixed 16-byte tag.

For AES-GCM, ``isKeyUpdateRequired()`` becomes true after :math:`2^{24}` successful records.
Protection and deprotection refuse record number 23,726,566, the floor of :math:`2^{24.5}`.
For ChaCha20-Poly1305 the signal becomes true while one old-key record remains, and processing refuses before advancing
a 64-bit sequence would wrap.

``updateApplicationTrafficKeys()`` performs only the caller-directed cryptographic transition from RFC 8446 Sections
4.6.3 and 7.2. Message parsing, serialization, and transition timing remain handshake responsibilities.
The method fully derives the next traffic secret, key, and IV before replacement, immediately erases the old generation,
and resets sequence and record counters to zero.

RFC 8446 Specification Mapping
------------------------------

The implementation follows the defining operations in review order:

* Section 7.1 encodes ``HkdfLabel`` as the requested network-order length, ``"tls13 "``-prefixed label, and empty
  context before HKDF expansion.
* Section 7.3 derives ``write_key`` and the 96-bit ``write_iv`` from ``Secret`` with the ``"key"`` and ``"iv"``
  labels.
* Section 5.3 encodes the 64-bit sequence in network byte order, left-pads it to 12 bytes, and XORs all 12 bytes with
  ``write_iv`` to form the nonce.
* Sections 5.2 and 5.4 encode content, the nonzero inner type, and explicit zero padding as ``TLSInnerPlaintext``.
* Section 5.2 encodes outer type 23, legacy version ``0x0303``, and ciphertext length. Those exact five bytes are
  ``additional_data`` for AEAD.
* Section 5.2 performs AEAD protection or deprotection through the symmetric API. Tentative decryption storage remains
  sensitive and guarded until tag verification succeeds.
* Section 5.4 reverse-scans authenticated plaintext over zero padding and validates the recovered inner type before
  copying content into its returned sensitive allocation.
* Section 5.5 checks record and sequence usage before processing and advances counters exactly once after success.
* Sections 4.6.3 and 7.2 derive ``"traffic upd"``, construct a complete replacement generation, erase the old secret,
  key, and IV, then reset the sequence.

Validation
----------

Focused tests include the RFC 8448 Section 3 AES-128-GCM application record and key derivation, plus pinned AES-256-GCM
and ChaCha20-Poly1305 composition vectors independently generated with Node.js 24.4.1 and OpenSSL 3.5.0. Boundary tests
cover sequences, content types, explicit padding, empty application data, maximum lengths, malformed framing, header and
ciphertext tampering, terminal erasure, usage ceilings, and KeyUpdate.
A bounded ASan/UBSan libFuzzer target seeds valid, malformed, oversized, and authentication-tampered records.

Specifications
--------------

* `RFC 8446: The Transport Layer Security (TLS) Protocol Version 1.3
  <https://www.rfc-editor.org/rfc/rfc8446.html>`_
* `RFC 8448: Example Handshake Traces for TLS 1.3 <https://www.rfc-editor.org/rfc/rfc8448.html>`_

Interface
=========

.. doxygenclass:: erbsland::cryptology::TlsCipherSuite
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfiguration
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationEntry
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationParser
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationResolution
    :members:
.. doxygenclass:: erbsland::cryptology::TlsServerIdentity
    :members:
.. doxygenclass:: erbsland::cryptology::TlsSignatureScheme
    :members:
.. doxygenenum:: erbsland::cryptology::TlsRecordContentType
.. doxygenclass:: erbsland::cryptology::TlsRecordDecryptor
    :members:
.. doxygenclass:: erbsland::cryptology::TlsRecordEncryptor
    :members:
.. doxygenclass:: erbsland::cryptology::TlsRecordError
    :members:
.. doxygenenum:: erbsland::cryptology::TlsRecordErrorCategory
.. doxygenclass:: erbsland::cryptology::TlsRecordPlaintext
    :members:
.. doxygenclass:: erbsland::cryptology::TlsTrafficSecret
    :members:
