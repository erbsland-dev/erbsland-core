..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Protected Data; Reference
    single: Protected Data
    single: ProtectedByteBlock
    single: Key Agreement
    single: X25519
    single: Application

**************
Protected Data
**************

Overview
========

:cpp:class:`ProtectedByteBlock <erbsland::cryptology::ProtectedByteBlock>` keeps application-lifetime secrets in an
opaque authenticated envelope.
The generic key-agreement API uses this storage for X25519 private keys and shared secrets.

Application Configuration
=========================

``setProtectedDataMode()`` selects how :cpp:class:`ProtectedByteBlock <erbsland::cryptology::ProtectedByteBlock>`
initializes its application-wide protection provider.
Call ``validateProtectedDataSupport()`` during startup when provider availability must become an explicit startup
failure.
Validation or the first non-empty protected block locks the mode; ``reset()`` preserves a locked mode.
The following sections describe provider behavior and lifecycle restrictions.

Protected Blocks
================

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

Interface
=========

.. doxygenclass:: erbsland::cryptology::ProtectedByteBlock
    :members:
.. doxygenenum:: erbsland::cryptology::ProtectedDataMode
