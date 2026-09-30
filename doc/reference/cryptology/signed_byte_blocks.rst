..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Signed Byte Blocks; Reference

******************
Signed Byte Blocks
******************

:cpp:class:`SignedByteBlock <erbsland::cryptology::SignedByteBlock>` stores a complete binary payload, its purpose,
an optional key-lookup hint, and an Ed25519 signature in one canonical record.
Generate a :cpp:class:`SigningPrivateKey <erbsland::cryptology::SigningPrivateKey>` with the ``Ed25519`` profile for
``sign()``.
Parsing requires a finite maximum payload length; ``verify()`` returns the payload only for a valid signature and exact
expected purpose.
``untrustedKeyIdHint()`` is for selecting a candidate verifier before that check.
See :doc:`/topics/cryptology/signing_and_verifying_byte_blocks` for the complete workflow, trust boundary, and
illustrated format.

Binary Format
=============

Version 1 is ``ELSB`` + ``0x01`` + Ed25519 algorithm ``0x01`` + one-byte key-ID length + one-byte purpose length +
four-byte big-endian payload length + key ID + UTF-8 purpose + payload + 64-byte Ed25519 signature.
The signature covers every preceding byte.
The key ID is absent or matches ``[_a-z0-9]{1,64}``; the purpose is one to 255 bytes.
Parsing rejects malformed lengths, invalid text, trailing data, and unsupported versions or algorithms.

Interface
=========

.. doxygenclass:: erbsland::cryptology::SignedByteBlock
    :members:
