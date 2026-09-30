..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Authentication Tokens; Reference

*********************
Authentication Tokens
*********************

:cpp:class:`AuthenticationTokenPair <erbsland::cryptology::AuthenticationTokenPair>` generates an Ed25519 client
credential and a matching server verifier from an administrator-chosen identifier.
The client credential is move-only and contains protected private material.
The server credential is copyable and contains only a public verification key.
``ServerAuthenticationToken::matches()`` can check a pair cryptographically during setup; the visible identifier alone
does not prove that two independently supplied keys match.
See :doc:`/topics/cryptology/authenticating_with_tokens` for the complete configuration and TCP workflow, security
boundaries, and interoperable wire format.

Credential Text
===============

``elctk-<id>-c-<data>`` contains the private client seed; ``elctk-<id>-s-<data>`` contains the public server key.
The ID matches ``[_a-z0-9]{1,64}`` and ``<data>`` is canonical, unpadded Base64url.
Parsers reject noncanonical encodings and role mismatches.

Challenge Lifecycle
===================

``createChallenge()`` returns a
:cpp:class:`PendingAuthenticationChallenge <erbsland::cryptology::PendingAuthenticationChallenge>` for one connection or
request.
The client returns a purpose-separated :cpp:class:`SignedByteBlock <erbsland::cryptology::SignedByteBlock>` containing
the exact challenge.
Any verification attempt consumes pending state, including a failed attempt.
The default lifetime is 60 seconds; a caller can request a shorter positive lifetime.

Interface
=========

.. doxygenclass:: erbsland::cryptology::AuthenticationTokenPair
    :members:
.. doxygenclass:: erbsland::cryptology::ClientAuthenticationToken
    :members:
.. doxygenclass:: erbsland::cryptology::PendingAuthenticationChallenge
    :members:
.. doxygenclass:: erbsland::cryptology::ServerAuthenticationToken
    :members:
