.. index::
    single: Signing Keys
    single: PKCS#8
    single: TLS Server Identity

************************************************
Protected Signing Keys and TLS Server Identities
************************************************

Private-Key Import
==================

:cpp:class:`SigningPrivateKey <erbsland::cryptology::SigningPrivateKey>` is the move-only entry point for TLS server
signing.
It imports one unencrypted PKCS#8 ``PrivateKeyInfo`` version 0 from canonical DER or one exact ``PRIVATE KEY`` PEM
block.
Raw private bytes are never exposed after import.

The importer accepts RFC 8410 Ed25519 seeds, RFC 5915 P-256 keys wrapped by ``id-ecPublicKey``, and RFC 8017 two-prime
RSA keys wrapped by ``rsaEncryption`` or a supported ``id-RSASSA-PSS`` identifier.
It rejects encrypted and legacy top-level containers, PKCS#8 attributes, trailing blocks or values, unsupported curves,
multiprime RSA, and algorithm parameters outside this profile.

Import validates the derived public value.
P-256 scalars must be exactly 32 octets in the range ``1..n-1``.
RSA moduli are limited to 2048--8192 bits, public exponents are odd and greater than 65536, and every modulus, exponent,
CRT exponent, and CRT coefficient relationship is checked before the normalized secret is transferred into
:cpp:class:`ProtectedByteBlock <erbsland::cryptology::ProtectedByteBlock>`.

Signing Algorithms
==================

:cpp:enum:`SigningKeyAlgorithm <erbsland::cryptology::SigningKeyAlgorithm>` distinguishes Ed25519, ECDSA P-256, and
RSA keys.
``supports()`` applies the complete TLS 1.3 key-family and RSA-PSS restriction matrix.
Certificate-only PKCS#1 signature schemes are never accepted for ``CertificateVerify``.

Pure Ed25519 follows RFC 8032 sections 5.1.5 and 5.1.6. Secret scalar multiplication uses a separate fixed 256-bit
schedule from the public verification path.
ECDSA follows FIPS 186-5 section 6.4.1 with P-256/SHA-256 and RFC 6979 section 3.2 deterministic nonces.
Its signature output is canonical DER.

RSA-PSS follows RFC 8017 sections 5.1.2, 8.1.1, and 9.1.1. SHA-256 and SHA-384 use digest-sized random salts.
Private exponentiation uses fixed-width Montgomery schedules, message blinding, and two-prime CRT acceleration.
Every result is verified with the cached public key before it leaves the signing operation.
Secret scalars, expanded seeds, nonces, RSA components, blinding values, and arithmetic scratch are guarded for erasure
on normal and exceptional exits.

Server Identities
=================

:cpp:class:`TlsServerIdentity <erbsland::cryptology::TlsServerIdentity>` combines a leaf-first ordered certificate
chain with its move-only signing key.
Construction rejects empty values, a leaf/key mismatch, or a key that supports no TLS 1.3 ``CertificateVerify`` scheme.
Ed25519 points compare exactly, P-256 points compare after affine normalization so compressed certificate keys are
accepted, and RSA matching includes modulus, exponent, key family, and PSS restrictions.

``selectSignatureScheme()`` examines ClientHello schemes in peer order and returns the first compatible value.
:cpp:class:`TlsConfiguration <erbsland::cryptology::TlsConfiguration>` stores an optional identity through immutable
shared ownership.
Configuration copies and application snapshots therefore remain copyable without copying private key material, and an
existing snapshot retains its identity after a later configuration is replaced or cleared.

File-loading helpers, encrypted and hardware-backed keys, key generation, multiple or SNI-selected identities, and the
TLS server protocol are intentionally outside this interface.

Interface
=========

.. doxygenenum:: erbsland::cryptology::SigningKeyAlgorithm
.. doxygenclass:: erbsland::cryptology::SigningPrivateKey
    :members:
.. doxygenclass:: erbsland::cryptology::TlsServerIdentity
    :members:
