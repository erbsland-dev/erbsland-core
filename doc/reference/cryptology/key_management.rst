..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Key Agreement; Reference
    single: Signing Keys; Reference
    single: Protected Data
    single: ProtectedByteBlock
    single: Key Agreement
    single: X25519
    single: Signing Keys
    single: PKCS#8
    single: TLS Server Identity

*************************
Key Agreement and Signing
*************************

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

X25519 and P-256 are supported.
X25519 follows RFC 7748. It clamps private scalars, accepts the exact 32-byte peer encodings permitted by RFC 7748,
masks the peer coordinate's top bit, and rejects an all-zero agreement result.
P-256 follows SEC 1 sections 3.2.1, 3.2.2.1, and 3.3.1 and RFC 8446 sections 4.2.8.2 and 7.4.2. Private scalars are
exactly 32 bytes and must lie in the interval ``[1,n-1]``.
Peer keys must be finite, on-curve, 65-byte uncompressed points; compressed TLS key shares are rejected.
Agreement retains leading zero octets in the fixed 32-byte x-coordinate result.
Private material and shared-secret results use protected storage with scoped plaintext access during computation.

Signing Keys
============

Key Generation and Import
-------------------------

:cpp:class:`SigningPrivateKey <erbsland::cryptology::SigningPrivateKey>` is the move-only entry point for TLS server
signing.
It generates Ed25519, ECDSA P-256/P-384, and two-prime RSA-2048/3072/4096 keys, or imports one unencrypted PKCS#8
``PrivateKeyInfo`` version 0 from canonical DER or one exact ``PRIVATE KEY`` PEM block.
The default :cpp:enum:`SigningKeyProfile <erbsland::cryptology::SigningKeyProfile>` is ECDSA P-256. The profile enum
deliberately fixes every algorithm parameter instead of exposing unsafe individual knobs.
Ed25519 generation uses a fresh 32-octet seed from application secure randomness.
Generic data signing is available only through the purpose-separated
:cpp:class:`SignedByteBlock <erbsland::cryptology::SignedByteBlock>` framing API.

The importer accepts RFC 8410 Ed25519 seeds, RFC 5915 P-256 and P-384 keys wrapped by ``id-ecPublicKey``, and RFC 8017
two-prime RSA keys wrapped by ``rsaEncryption`` or a supported ``id-RSASSA-PSS`` identifier.
It rejects encrypted and legacy top-level containers, PKCS#8 attributes, trailing blocks or values, unsupported curves,
multiprime RSA, and algorithm parameters outside this profile.

Import validates the derived public value.
P-256 and P-384 signing scalars must have the exact curve width and satisfy the extreme-scalar policy below.
RSA moduli are limited to 2048--8192 bits, public exponents are odd and greater than 65536, and every modulus, exponent,
CRT exponent, and CRT coefficient relationship is checked before the normalized secret is transferred into
:cpp:class:`ProtectedByteBlock <erbsland::cryptology::ProtectedByteBlock>`.

ECDSA generation uses full-width rejection sampling from application secure randomness and derives the public point with
a fixed secret-scalar schedule.
The library adds an extreme-scalar policy to the sampling mechanism described in FIPS 186-5 section A.2.2: for curve
width ``b`` and subgroup order ``n``, a signing scalar ``d`` must satisfy ``L <= d <= n-L``, where ``L = 2^(b-64)``.
The lower bound is therefore ``2^192`` for P-256 and ``2^320`` for P-384. Both boundaries are included.
This rejects tiny or severely truncated private values and values extremely close to the group order.
For uniformly sampled valid scalars, the probability of rejection by this additional policy is approximately ``2^-63``.
The check does not establish entropy or detect every predictable key.

Generation silently erases rejected candidates and draws again, allowing at most 128 total candidate draws.
A successful 128th candidate is accepted; rejecting all 128 throws
:cpp:class:`erbsland::random::RandomFailure <erbsland::random::RandomFailure>`, a fatal
:cpp:class:`erbsland::err::LogicFailure <erbsland::err::LogicFailure>`.
An entropy-source failure propagates immediately without retrying.
Import applies the same scalar policy before public-point derivation and rejects affected keys, including keys accepted
by earlier library versions, with :cpp:class:`erbsland::err::ParseError <erbsland::err::ParseError>`.
Non-throwing import methods return an empty key; encrypted imports preserve their generic decryption failure.
Import never substitutes a new key for the supplied identity.
This policy applies to ECDSA signing-key generation and import, not to low-level curve arithmetic, key agreement, or
ECDSA per-message nonces.

RSA generation uses ``e=65537``, exact-width random odd candidates, trial division, 25 Miller--Rabin rounds with fresh
random bases, prime-separation and private-exponent checks, CRT derivation, and a final public/private consistency
validation.
It uses the signing implementation's bounded fixed-width integer layer for secret arithmetic, not the general-purpose
decimal integer API.
The EC interval restriction is an additional library policy, not a FIPS requirement.
The library has not been submitted for FIPS validation and makes no certification claim.

Key Serialization and Files
---------------------------

``toDer()`` and ``toPem()`` reconstruct canonical unencrypted PKCS#8 while ``publicKey()`` returns an immutable
:cpp:class:`PublicKey <erbsland::cryptology::PublicKey>` that serializes as SubjectPublicKeyInfo DER or ``PUBLIC KEY``
PEM.
Private-key DER and PEM values are marked sensitive.
File writes use create-new behavior; private-key files additionally request user-only access.

:cpp:enum:`PemDerFormat <erbsland::cryptology::PemDerFormat>` is shared by keys, public keys, certificates, and
requests.
Automatic output recognizes ``.key`` /``.p8`` for private keys, ``.pub`` for public keys, and the common ``.pem`` and
``.der`` suffixes.
Unknown suffixes require an explicit format.

Password-Protected PKCS#8
-------------------------

Encrypted output uses RFC 8018 PBES2 with PBKDF2-HMAC-SHA-256, a fresh 16-octet salt, 1,000,000 iterations, and
AES-256-CBC with a fresh 16-octet IV and PKCS#7 padding.
The reader accepts this explicit profile with bounded input and an iteration limit of 10,000,000. Empty passwords are
rejected.
Password UTF-8 copies, derived keys, decrypted PKCS#8, padding buffers, and intermediate secret values are explicitly
erased, and every wrong-password or invalid-padding path reports only a generic decryption failure.

Signing Algorithms
------------------

:cpp:enum:`SigningKeyAlgorithm <erbsland::cryptology::SigningKeyAlgorithm>` distinguishes Ed25519, ECDSA P-256,
ECDSA P-384, and RSA keys.
``supports()`` applies the complete TLS 1.3 key-family and RSA-PSS restriction matrix.
Certificate-only PKCS#1 signature schemes are never accepted for ``CertificateVerify``.

Pure Ed25519 follows RFC 8032 sections 5.1.5 and 5.1.6. Secret scalar multiplication uses a separate fixed 256-bit
schedule from the public verification path.
ECDSA follows FIPS 186-5 section 6.4.1 with P-256/SHA-256 or P-384/SHA-384 and RFC 6979 section 3.2 deterministic
nonces.
Its signature output is canonical DER.

RSA-PSS follows RFC 8017 sections 5.1.2, 8.1.1, and 9.1.1. SHA-256, SHA-384, and SHA-512 use digest-sized random salts.
Private exponentiation uses fixed-width Montgomery schedules, message blinding, and two-prime CRT acceleration.
Every result is verified with the cached public key before it leaves the signing operation.
Secret scalars, expanded seeds, nonces, RSA components, blinding values, and arithmetic scratch are guarded for erasure
on normal and exceptional exits.

Raw key-import factories report incorrect byte lengths with ``OutOfRangeError``.
Invalid algorithm selections remain caller precondition failures.
Encrypted private-key imports reject empty passwords with ``CryptologyError``.

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
.. doxygenclass:: erbsland::cryptology::PublicKey
    :members:
.. doxygenenum:: erbsland::cryptology::SigningKeyAlgorithm
.. doxygenenum:: erbsland::cryptology::SigningKeyProfile
.. doxygenclass:: erbsland::cryptology::SigningPrivateKey
    :members:
.. doxygenenum:: erbsland::cryptology::PemDerFormat
