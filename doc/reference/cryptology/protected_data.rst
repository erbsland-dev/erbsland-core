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
    single: Secure Storage; Reference
    single: Persistent Secrets

**************
Protected Data
**************

Overview
========

:cpp:class:`ProtectedByteBlock <erbsland::cryptology::ProtectedByteBlock>` keeps application-lifetime secrets in an
opaque authenticated envelope.
The key-agreement API uses this storage for private keys and shared secrets.
:cpp:class:`SecureStorage <erbsland::cryptology::SecureStorage>` stores secrets across application runs through the current user's operating-system credential store.
Reading a stored secret returns a protected block for use in the current application.

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

Persistent Secret Storage
=========================

``SecureStorage`` provides synchronous read, write, and removal operations within an application namespace.
Namespace and item names are nonempty UTF-8 text, limited to 256 bytes each and without control characters.
The namespace separates application item names; operating-system access controls determine who may read them.
It does not isolate processes running as the same user.

``read()`` returns an optional ``ProtectedByteBlock``.
A missing item returns no value, while an existing empty item returns an empty protected block.
``write()`` creates or replaces an item from a protected block or borrowed plaintext bytes.
The borrowed overload leaves cleanup of the input with the caller.
The stored value is independent of the application's temporary protected-memory envelope, so it remains available to
later application instances.
``remove()`` reports whether the item existed.

Platform Support and Limits
---------------------------

``capabilities()`` reports platform support without accessing or unlocking storage.
macOS uses generic-password items in the current user's default standard Keychain, available to command-line and desktop
applications without data-protection-Keychain entitlements.
Items are local and are not marked for synchronization.
Windows uses generic Credential Manager items with current-user, local-computer persistence.
Linux reports unsupported.
The common payload limit is 2,560 bytes, including values read from items written by another program.

The operating system owns persistence, access controls, and any copies made by its security service.
The library erases scoped decrypted copies and mutable native password buffers when it releases them.
A failed update does not delete the previously stored value.

Interaction and Errors
----------------------

The default ``SecureStorageInteraction::Forbid`` policy suppresses authorization dialogs.
``SecureStorageInteraction::Allow`` permits them where supported; Windows generic-credential operations do not offer an
authorization dialog.
:cpp:class:`SecureStorageError <erbsland::cryptology::SecureStorageError>` distinguishes unsupported platforms, denied access, required interaction, cancellation, size
limits, invalid names, and native failures.
A missing item is not an error, but a retrieval failure is never treated as an empty value.

Operations may block, so call them during setup or on a worker when responsiveness matters.
On macOS, standard-Keychain interaction policy is process-wide.
The library serializes its own Keychain calls and restores the previous policy after each operation.
Applications making independent Security.framework calls must coordinate those calls and interaction-policy changes.

Configuration Integration
-------------------------

``text::placeholder::SecureStorageSource`` exposes items from one storage namespace as ``${secure:item}`` placeholders.
See :doc:`/reference/text/placeholders` for resolution and error behavior, and
:doc:`/topics/cryptology/configuring_tls` for a key-password configuration example.

Interface
=========

.. doxygenclass:: erbsland::cryptology::ProtectedByteBlock
    :members:
.. doxygenenum:: erbsland::cryptology::ProtectedDataMode
.. doxygenclass:: erbsland::cryptology::SecureStorage
    :members:
.. doxygenstruct:: erbsland::cryptology::SecureStorageCapabilities
    :members:
.. doxygenclass:: erbsland::cryptology::SecureStorageError
    :members:
.. doxygenenum:: erbsland::cryptology::SecureStorageErrorCategory
.. doxygenenum:: erbsland::cryptology::SecureStorageInteraction
