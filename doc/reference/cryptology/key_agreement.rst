..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Protected Data
    single: ProtectedByteBlock
    single: Key Agreement
    single: X25519

********************************
Protected Data and Key Agreement
********************************

:cpp:class:`ProtectedByteBlock <erbsland::cryptology::ProtectedByteBlock>` keeps application-lifetime secrets in an
opaque authenticated envelope.
The generic key-agreement API uses this storage for X25519 private keys and shared secrets.

Protected Data
==============

A protected block stores only provider-owned ciphertext and the authenticated plaintext length.
It never retains a provider pointer, key, native key reference, or registry entry.
Copying, moving, destroying, or reading its metadata therefore does not resolve the application protection service.
Use ``unprotect()`` when you need an owning sensitive byte block, or ``withUnprotectedData()`` to limit plaintext access
to one callback.
The callback storage is erased after a normal return and while an exception unwinds.

Protection is memory hardening for the lifetime of one :cpp:class:`Application <erbsland::core::Application>`.
It is not a persistent file format, a process isolation boundary, or protection from arbitrary code already executing
inside the process.
An envelope cannot be decrypted after its application is destroyed or by a later application instance.
Empty protected blocks do not require an application and never initialize a provider.

Provider Selection
==================

:cpp:enum:`ProtectedDataMode <erbsland::cryptology::ProtectedDataMode>` controls provider selection before the first
non-empty block or an explicit ``validateProtectedDataSupport()`` call locks the choice.
``Automatic`` prefers the native provider on macOS and Windows, performs an authenticated round-trip self-test, and
falls back to internal AES-256-GCM only during initialization.
``InternalOnly`` avoids native detection entirely.
``PlatformOnly`` turns an unavailable or failing native provider into a
:cpp:class:`CryptologyError <erbsland::cryptology::CryptologyError>`.
After lock-in, provider errors fail closed and never switch the provider behind existing envelopes.

On macOS, native protection uses an application-lifetime Secure Enclave P-256 key and Security framework ECIES/AES-GCM.
The process must have access to the required keychain facilities; otherwise ``Automatic`` uses the internal provider.
On Windows, native protection uses DPAPI-NG with a logon-local descriptor held by the application service.
The provider loads ``ncrypt.dll`` from the Windows system directory only when selected, resolves the four required entry
points explicitly, and releases the library after closing its descriptor.
Linux deliberately uses the bundled AES-256-GCM provider for ``Automatic`` and ``InternalOnly``.
It does not probe AF_ALG, kernel keyrings, TPM services, or desktop key stores, and ``PlatformOnly`` fails.

The internal provider generates one AES-256 key per application, assigns a unique counter nonce to each envelope, and
authenticates envelope metadata.
Application destruction releases or deletes native references and securely erases an internally owned key.

Key Agreement
=============

:cpp:class:`KeyAgreementPrivateKey <erbsland::cryptology::KeyAgreementPrivateKey>` is move-only and stores its private
scalar in a protected block.
It caches the ordinary :cpp:class:`KeyAgreementPublicKey <erbsland::cryptology::KeyAgreementPublicKey>`, so accessing
the public key does not decrypt private material.
Create a key with ``generate()`` or import exact private bytes with ``fromBytes()``.
There is intentionally no raw private-key accessor.

``agree()`` accepts a public key with the same
:cpp:class:`KeyAgreementAlgorithm <erbsland::cryptology::KeyAgreementAlgorithm>` and returns a move-only
:cpp:class:`KeyAgreementSharedSecret <erbsland::cryptology::KeyAgreementSharedSecret>`.
Pass that shared secret directly to :cpp:class:`Hkdf <erbsland::cryptology::Hkdf>` ``extract()`` to derive keying
material through scoped plaintext access.

X25519 is the first supported algorithm.
It clamps private scalars, accepts the exact 32-byte peer encodings permitted by RFC 7748, masks the peer coordinate's
top bit, and rejects an all-zero agreement result.
P-256 and P-384 can be added through the same generic dispatch later.
Platform-backed key-agreement private-key handles are not part of this API milestone; native references here belong only
to protected-data providers.

Interface
=========

.. doxygenclass:: erbsland::cryptology::KeyAgreementAlgorithm
    :members:
.. doxygenclass:: erbsland::cryptology::KeyAgreementPrivateKey
    :members:
.. doxygenclass:: erbsland::cryptology::KeyAgreementPublicKey
    :members:
.. doxygenclass:: erbsland::cryptology::KeyAgreementSharedSecret
    :members:
.. doxygenclass:: erbsland::cryptology::ProtectedByteBlock
    :members:
.. doxygenenum:: erbsland::cryptology::ProtectedDataMode
