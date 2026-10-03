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
    single: TLS Identity
    single: System Identity Store
    single: TLS Record Protection
    single: TLS 1.3

***********
TLS Support
***********

Configuration
=============

:cpp:class:`TlsConfiguration <erbsland::cryptology::TlsConfiguration>` groups independent optional :cpp:class:`TlsClientConfiguration <erbsland::cryptology::TlsClientConfiguration>` and :cpp:class:`TlsServerConfiguration <erbsland::cryptology::TlsServerConfiguration>` parts.
Each part holds its own trust selection and shares an immutable identity.
:cpp:class:`TlsTrustConfiguration <erbsland::cryptology::TlsTrustConfiguration>` describes profile trust; :cpp:class:`TlsSettings <erbsland::cryptology::TlsSettings>` supplies application-wide defaults and native
options.
See :doc:`/topics/cryptology/configuring_tls` for configuring services, loading identities, and reloading profiles.

Registration and Replacement
----------------------------

:cpp:func:`CryptologyConfiguration::setTlsConfiguration() <erbsland::cryptology::CryptologyConfiguration::setTlsConfiguration>` replaces each supplied role completely, preserving the other role
already registered at that label.
It validates supplied roles against the installed shared settings and does not merge fields within a role.
:cpp:func:`CryptologyConfiguration::replaceTlsConfigurations() <erbsland::cryptology::CryptologyConfiguration::replaceTlsConfigurations>` validates an entire :cpp:class:`TlsConfigurationSet <erbsland::cryptology::TlsConfigurationSet>` before replacing its
shared settings and named profiles atomically.
A failed replacement leaves the installed set unchanged.

:cpp:func:`CryptologyConfiguration::hasTlsConfiguration() <erbsland::cryptology::CryptologyConfiguration::hasTlsConfiguration>` tests exact presence for one role.
:cpp:func:`CryptologyConfiguration::clearTlsConfiguration() <erbsland::cryptology::CryptologyConfiguration::clearTlsConfiguration>` removes one named role, or both when no role is specified.
:cpp:func:`CryptologyConfiguration::clearTlsConfigurations() <erbsland::cryptology::CryptologyConfiguration::clearTlsConfigurations>` preserves shared settings and built-in defaults.
:cpp:func:`CryptologyConfiguration::reset() <erbsland::cryptology::CryptologyConfiguration::reset>` also restores shared TLS defaults.
Connections retain their selected immutable profiles and effective policies across registry changes.

Label Resolution
----------------

:cpp:func:`CryptologyConfiguration::resolveTlsConfiguration() <erbsland::cryptology::CryptologyConfiguration::resolveTlsConfiguration>` searches only the requested :cpp:enum:`TlsRole <erbsland::cryptology::TlsRole>` at the exact label, its
slash-delimited ancestors, and finally the reserved empty label.
The returned :cpp:class:`TlsConfigurationResolution <erbsland::cryptology::TlsConfigurationResolution>` contains
requested and matched labels, the role, the immutable configuration, and its effective certificate policy.
Resolution performs no file access or native trust evaluation.
An invalid selected role fails without trying an ancestor.
:cpp:func:`CryptologyConfiguration::resolveTlsConfigurations() <erbsland::cryptology::CryptologyConfiguration::resolveTlsConfigurations>` captures multiple labels from one registry state, allowing a
server to select its default and SNI profiles consistently.

Both built-in defaults always exist and cannot be registered, replaced, or individually cleared.
The client default has no identity and selects system trust subject to shared settings; all unresolved client labels
reach it.
The server default has no identity and fails synchronously when resolved.
Low-level TLS connections request ``tls`` and HTTP requests ``http`` in both roles.

Named labels contain at most 16 nonempty slash-separated segments and 255 bytes, using lowercase ASCII letters, digits,
and interior hyphens.
At most 256 named labels may be installed; a label containing both roles counts once and defaults consume no capacity.

Trust Selection and Shared Settings
-----------------------------------

:cpp:func:`TlsTrustConfiguration::resolve() <erbsland::cryptology::TlsTrustConfiguration::resolve>` applies trust selection in this order:

#. The profile's explicit ``System Trust`` value.
#. Explicit anchors in the profile, which select explicit trust.
#. The shared ``System Trust`` value, if specified.
#. The role default: system trust for clients, explicit trust for servers.

Profile system trust cannot be combined with explicit anchors, and intermediates require explicit anchors.
Clients always require an effective trust source.
Servers require one when :cpp:enum:`TlsClientAuthentication <erbsland::cryptology::TlsClientAuthentication>` is
``Optional`` or ``Required``.
Disabling shared system trust without supplying client anchors causes resolution to fail, never unverified acceptance.

The following ELCL fields belong to ``[TLS]`` and map to :cpp:class:`TlsSettings <erbsland::cryptology::TlsSettings>`:

.. list-table::
    :header-rows: 1
    :widths: 30 25 45

    * - Field
      - Default
      - Effect
    * - ``System Trust``
      - Unspecified
      - Override role defaults unless the profile selects its own trust source.
    * - ``Trust Allow Network``
      - ``true``
      - Allow native trust evaluation to retrieve intermediate certificates and revocation information.
    * - ``Trust Bundle File``
      - Automatic discovery
      - Override the Linux system CA bundle.
    * - ``Identity Allow Interaction``
      - ``false``
      - Permit native identity providers to prompt during loading and signing.

A bundle override on another platform fails when a system-trust policy is evaluated.
Identity interaction permission is captured at loading and remains attached to the signer.
Changing that permission requires reloading the identity.
See :doc:`x509_certificates` for platform trust behavior and revocation handling.

ELCL Parsing and Validation
---------------------------

:cpp:func:`TlsConfigurationParser::parse() <erbsland::cryptology::TlsConfigurationParser::parse>` parses the complete optional TLS section into an owned :cpp:class:`TlsConfigurationSet <erbsland::cryptology::TlsConfigurationSet>`.
The schema has shared ``[TLS]`` settings and optional ``*[TLS . Client]*`` and ``*[TLS . Server]*`` section lists.
A label may occur once per role; matching labels across the two lists combine into one configuration.
An absent TLS section returns a default set.
ELCL names are case-insensitive and treat spaces and underscores equivalently.

:cpp:func:`TlsConfigurationParser::validationRules() <erbsland::cryptology::TlsConfigurationParser::validationRules>` and :cpp:func:`TlsConfigurationParser::version() <erbsland::cryptology::TlsConfigurationParser::version>` expose the compiled schema and
its version for a separate validation pass.
Parsing always repeats schema validation, then loads certificate and identity material and checks profile trust.
Validation alone does not load files or verify certificate/key matches.
Password fields receive secret annotations even when supplied as ordinary ELCL text.

Relative material paths resolve from the file defining the field, including fields supplied by an included file.
Absolute paths are accepted as written.
Text and stream sources require absolute paths.
Loading errors are reported as :cpp:class:`ConfError <erbsland::conf::ConfError>` with the responsible source location
and the underlying error as a cause where available.

TLS Identities
==============

:cpp:class:`TlsIdentity <erbsland::cryptology::TlsIdentity>` combines a leaf-first certificate chain with a signer backed by a software key or an operating-system
provider.
The software-key constructor takes a move-only :cpp:class:`SigningPrivateKey <erbsland::cryptology::SigningPrivateKey>`.
Construction rejects an empty chain, a leaf/key mismatch, or a key with no supported TLS 1.3 signature scheme.
Identity chains are limited to 16 certificates and a one-MiB encoded TLS Certificate message.
Mismatches and unsupported schemes report ``CryptologyError``; empty components are caller precondition failures.

:cpp:func:`TlsConfiguration::setServer() <erbsland::cryptology::TlsConfiguration::setServer>` and ``setClient()`` configure independent role parts.
Their identities use immutable shared ownership.
Configuration copies and connection snapshots share the identity without copying private key material.
Replacing or clearing a registry entry does not invalidate existing snapshots.
``selectSignatureScheme()`` examines the peer's offered schemes in order and returns the first compatible value.

Client Certificate Authentication
---------------------------------

A client identity is sent only in response to an initial CertificateRequest, after the server's certificate,
CertificateVerify, and Finished messages have authenticated it.
The identity must satisfy certificate validity, digital-signature key usage, client-authentication purpose, requested
issuer names, and signature-scheme requirements.
Unsupported nonempty OID filters suppress identity disclosure.
An absent or unsuitable identity produces an empty certificate list; the connection never searches a store for another
identity.
:cpp:func:`TlsServerConfiguration::setClientAuthentication() <erbsland::cryptology::TlsServerConfiguration::setClientAuthentication>` selects whether the server requests a client certificate during
the initial handshake:

.. list-table::
    :header-rows: 1
    :widths: 20 80

    * - Mode
      - Behavior
    * - ``Disabled``
      - Send no certificate request. This is the default.
    * - ``Optional``
      - Request a certificate and accept an empty certificate list as anonymous.
    * - ``Required``
      - Request a certificate and reject an empty certificate list with ``certificate_required``.

Both enabled modes reject invalid presented certificates or proofs of private-key possession.
CertificateRequest uses an empty context and supported signature algorithms without CA-name hints or OID filters.
Client path validation, CertificateVerify, and Finished must all succeed before authenticated peer metadata is
published.
See :doc:`/reference/network/connections` for server connection metadata and its lifetime.
Post-handshake authentication, PSK/resumption, and early data are not supported.

Native Identity Stores
----------------------

:cpp:func:`SystemIdentityStore::list() <erbsland::cryptology::SystemIdentityStore::list>` returns public :cpp:struct:`SystemIdentityInfo <erbsland::cryptology::SystemIdentityInfo>` metadata.
``load()`` selects an identity by its exact 32-byte SHA-256 leaf-certificate fingerprint and returns a
:cpp:class:`TlsIdentity <erbsland::cryptology::TlsIdentity>` that retains the native signer without exporting the
private key.
Public intermediate certificates are collected from locally available material, within the identity chain limit.
Enumeration is limited to 4,096 entries.

macOS uses identities from the current user's Keychain.
Windows uses the current-user personal certificate store with CNG private keys; legacy CAPI keys and machine stores are
not supported.
``isSupported()`` reports whether a native provider is available; Linux applications can use file-based identities.
Provisioning certificates and private keys remains a deployment task.

Store enumeration and identity loading are synchronous, so perform them during setup or on an appropriate worker.
Connection-triggered signing runs on background workers as described in :doc:`/reference/network/connections`.
``SystemIdentityStore{true}`` permits provider authorization dialogs; the default is silent access.
On macOS, Keychain interaction policy is process-wide.
The library serializes its own Keychain calls and restores the previous policy after each call; applications making
independent Security.framework calls must coordinate their use of that policy.

See :doc:`/topics/cryptology/configuring_tls` for configuring file-based and native identities in ELCL.

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
An incorrect input length reports ``OutOfRangeError``.
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

Programmatic TLS profile registration and resolution raise ``CryptologyError`` for unusable profiles, conflicting trust
settings, duplicate roles, or registry capacity exhaustion.
Invalid label syntax and role values are caller precondition failures.
Linux trust-bundle discovery and unstable snapshots also raise ``CryptologyError``; file access and certificate parsing
retain their documented path, stream, and parse errors.

Interface
=========

.. doxygenstruct:: erbsland::cryptology::SystemIdentityInfo
    :members:
.. doxygenclass:: erbsland::cryptology::SystemIdentityStore
    :members:
.. doxygenclass:: erbsland::cryptology::TlsCipherSuite
    :members:
.. doxygenenum:: erbsland::cryptology::TlsClientAuthentication
.. doxygenclass:: erbsland::cryptology::TlsClientConfiguration
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfiguration
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationEntry
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationParser
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationResolution
    :members:
.. doxygenclass:: erbsland::cryptology::TlsConfigurationSet
    :members:
.. doxygenclass:: erbsland::cryptology::TlsIdentity
    :members:
.. doxygenenum:: erbsland::cryptology::TlsRole
.. doxygenclass:: erbsland::cryptology::TlsServerConfiguration
    :members:
.. doxygenclass:: erbsland::cryptology::TlsSettings
    :members:
.. doxygenclass:: erbsland::cryptology::TlsSignatureScheme
    :members:
.. doxygenclass:: erbsland::cryptology::TlsTrustConfiguration
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
