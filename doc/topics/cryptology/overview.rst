..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    !single: Cryptology Overview
    single: Cryptology
    single: Cryptographic Hash
    single: Hash Algorithm
    single: Message Digest

.. _cryptology-overview:

*******************
Cryptology Overview
*******************

This page gives you an overview of the cryptology domain in Erbsland Core.
You will learn where to find safe password storage, sensitive-data, and general hashing guidance.

Cryptographic APIs protect different properties of data.
Hash algorithms create a fixed-size representation of a message, encryption protects confidentiality, signatures
establish authenticity, and certificates connect identities to public keys.
These mechanisms often work together, but they are not interchangeable.
Start with the security property your application needs and then select the matching topic and API.

Storing and Verifying Passwords
===============================

Passwords need a deliberately slow, salted password derivation, not a general hash.
The :cpp:class:`PasswordHasher <erbsland::cryptology::PasswordHasher>` API accepts
:cpp:type:`String <erbsland::text::String>`, recommends marking password input as sensitive, requires an application key
on the normal path, and produces an opaque :cpp:class:`PasswordHash <erbsland::cryptology::PasswordHash>` record.
Successful verification can include a replacement record when costs, algorithms, or keys need migration.

See :doc:`storing_and_verifying_passwords` for the complete storage and login workflow and
:doc:`supported_password_hashing_algorithms` for Argon2id, scrypt, presets, and migration policy.

Sensitive Data
==============

A marked :cpp:type:`String <erbsland::text::String>` uses securely erased shared UTF-8 storage.
Binary secrets use :cpp:class:`ByteBlock <erbsland::mem::ByteBlock>` and
:cpp:class:`ByteBlockEditor <erbsland::mem::ByteBlockEditor>` allocations marked as sensitive.
These mechanisms reduce recoverable heap remnants and keep secret-handling intent visible in APIs.
They do not lock memory, prevent swapping, or erase ordinary source objects.
See :doc:`/topics/security/about_sensitive_strings_and_byte_blocks` for their intended uses, ownership behavior, and
security limitations.

Using Hash Algorithms
=====================

A cryptographic hash maps an arbitrary byte sequence to a fixed-size digest.
Applications use digests to identify content, detect changes when the expected digest is trusted, and prepare data for
higher-level cryptographic operations.

The :cpp:class:`Hasher <erbsland::cryptology::Hasher>` API processes data incrementally, so the complete message does
not have to be held in memory.
:cpp:class:`HashAlgorithm <erbsland::cryptology::HashAlgorithm>` represents an explicit algorithm and can also select a
recommendation from application requirements.

See :doc:`using_hash_algorithms` to learn how to hash data, select an algorithm, persist digests, and defend code that
processes untrusted input.

Reading X.509 Certificates
==========================

:cpp:class:`X509Certificate <erbsland::cryptology::X509Certificate>` reads a single certificate from PEM, DER, or a
file and exposes its names, validity period, public-key container, extensions, signature bytes, and exact encoded form.
:cpp:class:`X509CertificateBundle <erbsland::cryptology::X509CertificateBundle>` handles ordered PEM files containing
multiple certificates.
Parsing verifies canonical encoding and the supported certificate profile, but does not establish trust, validate a
signature, match a hostname, or decide whether a certificate is acceptable for a connection.
See :doc:`/reference/cryptology/x509_certificates` for formats, strict and compatible parsing, resource limits, and the
typed field API.

Creating TLS Certificates and Requests
======================================

The profile-driven :cpp:class:`X509CertificateBuilder <erbsland::cryptology::X509CertificateBuilder>` creates a local
test CA and its server or client certificates, or produces a PKCS#10 request for an enterprise CA. It supplies the
extension and signature defaults that are easy to get wrong while leaving application identity, SANs, validity, and
hierarchy policy explicit.
See :doc:`creating_tls_certificates` for complete test and enterprise workflows.

Configuring TLS Profiles
========================

Applications can read labeled client and server TLS profiles from ELCL section lists.
:cpp:class:`TlsConfigurationParser <erbsland::cryptology::TlsConfigurationParser>` validates one list entry, loads its
certificate and key files, and returns an independent
:cpp:class:`TlsConfigurationEntry <erbsland::cryptology::TlsConfigurationEntry>` for registration.
See :doc:`configuring_tls` for client-only, server-only, combined, fallback, and included-file examples.

Supported Hash Algorithms
=========================

Erbsland Core supports the SHA-3 and SHA-2 families for new cryptographic results.
SHA-1 and MD5 remain available only to read or verify legacy formats and are rejected by the normal safety policy.

See :doc:`supported_hash_algorithms` for the complete catalog, security strengths, intended uses, and differences
between related algorithms.
