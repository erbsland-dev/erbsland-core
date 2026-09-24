..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Document Signatures
    single: Document Signatures; Trust Anchors
    single: ELCL; @signature
    single: Security; Configuration Integrity

***********************************
Trusting Signed Configuration Files
***********************************

A configuration signature lets your application detect whether a document changed after an approved person or system
signed it.
This is useful when configuration files pass through several computers, deployment stages, or administrators before the
application reads them.
This page explains what a signature can establish, why the trust policy matters more than the cryptographic operation,
and how ELCL and Erbsland Core divide the work between the parser and your application.

Why Sign Configuration Files?
=============================

Many configuration errors are innocent: a value is changed during an investigation, a temporary workaround survives into
production, or a file from the wrong deployment is copied onto a server.
In a large environment, the maintenance team may also need to distinguish a software defect from an unrecorded local
change.
A valid signature provides a clear checkpoint: the signed bytes are the bytes an identified signer approved.

The same mechanism also detects deliberate modification by someone who cannot produce an accepted signature.
This makes signatures useful for release configurations, security-sensitive policy files, and appliances administered
outside the development team.
The signer identity and signing time can additionally support an audit trail when your signature format records them.

A signature does not encrypt the document or hide its values.
It also does not decide whether a signer was *allowed* to approve this particular configuration.
Cryptography establishes that a key signed a digest; your trust policy gives that fact its application meaning.

The Trust Anchor Is the Hard Part
=================================

Creating and verifying a digital signature is a well-defined operation.
The difficult question is where your application gets the independent knowledge needed to trust the signing key.
That knowledge is the *trust anchor*.
It must be available before the application accepts the configuration, and it must not be replaceable by editing the
same configuration that it protects.

S/MIME certificates provide one practical model.
The signer uses the private key associated with an S/MIME certificate, while the application validates the certificate
chain using its local certificate store and trusted root certificate authorities.
The certificate can connect the signing key to a name and email address, and the signature can keep that identity
visible beside the document.

.. mermaid::

    flowchart LR
        Root[Trusted root CA] -->|validates| Certificate[Signer's S/MIME certificate]
        Certificate -->|identifies public key| Signature[Document signature]
        Document[Configuration bytes] --> Digest[Document digest]
        Digest --> Signature
        Signature --> Result[Integrity and signer identity established]

For some applications, this is enough.
If the goal is to detect unnoticed edits and record who approved the file, accepting any currently valid certificate
from the chosen certificate authorities may provide the intended assurance.
The readable name or email address in the signature can then help operators understand who signed the configuration.

Identity Is Not Authorization
-----------------------------

A valid certificate can identify its holder without granting that person permission to alter your application's
configuration.
Public certificate authorities may issue valid certificates to many people, and even an internal authority may serve an
organization much larger than the group that administers one system.
Restricting signatures to selected people therefore requires a second policy: an allowlist, a dedicated issuing
authority, an embedded public key, or another rule that the application trusts.

Keeping that allowlist inside an ordinary configuration file does not solve the problem.
Anyone who can replace both files could authorize a new identity and then sign the protected document with that
identity.
The authorization decision needs an anchor outside the editable configuration set.

.. mermaid::

    flowchart TB
        subgraph Unsafe[Self-contained policy]
            Editable[Editable configuration] --> Allowlist[Editable signer allowlist]
            Allowlist --> DecisionA[Authorization decision]
            Editable -. attacker can replace both .-> DecisionA
        end
        subgraph Trusted[Independent policy]
            Anchor[Embedded CA, public key, or protected license] --> Authorized[Authorized signer identities]
            Signed[Signed configuration] --> DecisionB[Authorization decision]
            Authorized --> DecisionB
        end

An application can embed a dedicated CA certificate or public key in its executable.
It can also reuse an existing license or entitlement system that securely associates a deployment with the identities
allowed to sign its configuration.
An operating-system certificate store can establish certificate validity, but a separate application rule is still
needed when only a subset of otherwise valid certificate holders is authorized.

Keep Strict Enforcement for Deployment
--------------------------------------

Signing every small edit slows down development and diagnosis, especially when a private key lives on an external device
or signing service.
A practical design therefore makes strict signature enforcement an explicit deployment policy.
Developers can work with unsigned files in a controlled environment, then sign the final production configuration and
enable the production trust policy as the last deployment step.

This choice must itself be protected.
If an attacker can disable enforcement through the configuration being verified, the signature boundary provides no
security.
Select the policy through trusted application state, a protected license, a build choice, or another mechanism outside
the document.

How ELCL Carries a Signature
============================

ELCL reserves the ``@signature`` meta-value for an application-defined signature payload.
It must be a quoted text value on the first line of the document:

.. code-block:: elcl

    @signature: "admin@example.org;2026-09-24T14:30:00Z;SHA3-256;<signature data>"

    [service]
    listen address: "127.0.0.1"
    port: 8080

The language deliberately does not prescribe one certificate system or signature-text format.
Your application may store an email address, signing time, algorithm identifier, certificate reference, and encoded
signature in that text, or use another format suited to its trust model.
The validator and signer must agree on the format.

The signed content begins on the second line.
The signature line itself is excluded so that an existing signature can be replaced without changing the digest it
describes.
Every other byte matters, including comments, spacing, and line endings.
Editing the document therefore invalidates its signature even when the parsed value tree would remain the same.
Remove the signature line while editing and generate a new signature after the final version has been checked.

How Erbsland Core Divides the Work
==================================

Erbsland Core handles document reading, digest calculation, and the placement of the ELCL signature line.
Your application handles keys, certificates, signature algorithms, and the trust policy.
This separation keeps the parser independent from a particular public-key infrastructure while ensuring that signing and
parsing use exactly the same document bytes.

Validating While Parsing
------------------------

Install a :cpp:class:`SignatureValidator <erbsland::conf::SignatureValidator>` with
:cpp:func:`Parser::setSignatureValidator() <erbsland::conf::Parser::setSignatureValidator>`.
For every document, including unsigned and included documents, the parser supplies the source identifier, the raw
signature text, and its document digest to ``SignatureValidator::validate()``.
The validator interprets the signature and applies your trust policy, then returns
:cpp:enumerator:`SignatureValidatorResult::Accept <erbsland::conf::SignatureValidatorResult::Accept>` or
:cpp:enumerator:`SignatureValidatorResult::Reject <erbsland::conf::SignatureValidatorResult::Reject>`.

This callback for unsigned documents is important: an empty signature text lets the same policy accept unsigned files in
a development mode and reject them in production.
Without a validator, the parser accepts unsigned documents but rejects signed documents because it cannot establish
their authenticity.

The next topic, *Validating Document Signatures*, builds a validator and shows both successful and rejected parse
operations.

Signing a File
--------------

Implement :cpp:class:`SignatureSigner <erbsland::conf::SignatureSigner>` to turn the parser-compatible document digest
and signer information into your signature text.
Pass that implementation to :cpp:class:`Signer <erbsland::conf::Signer>`, which reads an input file and writes a copy
with a new first-line ``@signature`` value.
An existing signature line is replaced, and the original line-ending convention is preserved.

The signing tool checks the document encoding and line limits, but it does not validate ELCL syntax.
Parse and validate the unsigned document first, then sign the exact file that will be deployed.
The following topic, *Signing Configuration Documents*, develops this workflow and compares ways to integrate it into an
application, a separate local tool, or a web service.
