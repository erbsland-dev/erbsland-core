..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: HMAC
    single: HKDF
    single: Message Authentication
    single: Key Derivation

*****************************************
Message Authentication and Key Derivation
*****************************************

:cpp:class:`Hmac <erbsland::cryptology::Hmac>` implements streaming HMAC with SHA-256 or SHA-384.
:cpp:class:`Hkdf <erbsland::cryptology::Hkdf>` implements the generic RFC 5869 extract and expand operations for the
same hash algorithms.

Message Authentication
======================

An HMAC state retains secret key material and is therefore move-only.
It accepts segmented bytes or the exact stored UTF-8 bytes of a string, caches the complete authenticator after
finalization, and can reset to authenticate another message with the same key.
``verify()`` accepts only a complete hash-sized authenticator; equal-length values are compared without
content-dependent short-circuiting.
``secureErase()`` immediately erases retained keyed and message-dependent state and leaves an invalid placeholder.

Key Derivation
==============

An HKDF value stores only its selected hash algorithm.
``extract()`` returns a digest-sized pseudorandom key and ``expand()`` returns the requested output keying material.
Every non-empty result uses sensitive storage that is securely erased when its final shared reference is released.
An empty salt follows RFC 5869 and acts as a digest-sized all-zero salt.
The shared-secret overload accepts a
:cpp:class:`KeyAgreementSharedSecret <erbsland::cryptology::KeyAgreementSharedSecret>` and performs extraction through
scoped plaintext access.
Expansion requires a pseudorandom key at least as long as the digest and accepts at most 255 digest blocks: 8,160 bytes
for SHA-256 or 12,240 bytes for SHA-384.

Interface
=========

.. doxygenclass:: erbsland::cryptology::Hkdf
    :members:
.. doxygenclass:: erbsland::cryptology::Hmac
    :members:
