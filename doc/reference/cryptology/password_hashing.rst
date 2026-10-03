..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Cryptology
    single: PasswordHasher
    single: PasswordHash
    single: PasswordHashKey

****************
Password Hashing
****************

:cpp:class:`PasswordHasher <erbsland::cryptology::PasswordHasher>` is the safe entry point for creating and verifying
password records.
It requires sensitive password input and normally requires an application key.

Storage Values and Verification
===============================

:cpp:class:`PasswordHash <erbsland::cryptology::PasswordHash>` is an immutable, strictly parsed storage value.
:cpp:class:`PasswordVerification <erbsland::cryptology::PasswordVerification>` reports an explicit accepted or
rejected state and can carry a replacement record after a successful migration.
See :doc:`/topics/cryptology/storing_and_verifying_passwords` for the complete workflow.

Policies and Keys
=================

:cpp:class:`PasswordHashPolicy <erbsland::cryptology::PasswordHashPolicy>` provides reviewed Argon2id and scrypt
presets.
:cpp:class:`PasswordHashKey <erbsland::cryptology::PasswordHashKey>` holds the application pepper and can carry a
public identifier for rotation.
Use ``PasswordHashKey::isValidIdentifier`` to validate identifiers read from configuration before constructing a key.
Identifiers must contain 1-32 ASCII letters, digits, periods, underscores, or hyphens.
See :doc:`/topics/cryptology/supported_password_hashing_algorithms` for construction details, parameters, and migration
policy.

The ``PasswordHasher::UnsafeNoPasswordHashKey{}`` tag explicitly selects hashing without an application pepper.
The empty tag is a public nested struct of ``PasswordHasher`` in ``<erbsland/PasswordHasher.hpp>``.
Pass it directly to the constructor when this reduced protection is intentional.

Interface
=========

.. doxygenclass:: erbsland::cryptology::PasswordHash
    :members:
.. doxygenclass:: erbsland::cryptology::PasswordHashAlgorithm
    :members:
.. doxygenclass:: erbsland::cryptology::PasswordHasher
    :members:
.. doxygenclass:: erbsland::cryptology::PasswordHashKey
    :members:
.. doxygenclass:: erbsland::cryptology::PasswordHashPolicy
    :members:
.. doxygenclass:: erbsland::cryptology::PasswordVerification
    :members:
.. doxygenclass:: erbsland::cryptology::unsafe::UnsafeCustomPasswordHashParameters
    :members:
