..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Signing Documents
    single: Document Signatures; Signing
    single: SignatureSigner
    single: Signer
    single: Security; Signing Workflow

*******************************
Signing Configuration Documents
*******************************

Signing marks the exact configuration file that an authorized person or system approved.
Erbsland Core provides :cpp:class:`Signer <erbsland::conf::Signer>` for the document mechanics and
:cpp:class:`SignatureSigner <erbsland::conf::SignatureSigner>` as the boundary to your keys or signing service.
This page shows how to implement that boundary, how to validate before signing, and how to choose a signing workflow
that fits local development, controlled deployments, and restricted enterprise environments.

What the Signing Tool Does
==========================

``Signer`` reads the source as UTF-8, enforces the ELCL line limit, calculates the same document digest as the parser,
and passes that digest to ``SignatureSigner::sign()``.
It then writes a copy with an ``@signature`` meta-value on the first line.
When the source already has a signature line, the old line is excluded from the digest and replaced in the destination.
The original line-ending convention is preserved.

The signing operation deliberately does not parse ELCL syntax or apply validation rules.
This separation lets the signer work with the exact bytes that will be deployed, but it also means that a successful
``Signer::sign()`` call says nothing about whether the configuration is usable.
Parse and validate the source first, and do not modify the output after signing.

Implementing a Signing Backend
==============================

The parser-compatible digest arrives in
:cpp:struct:`SignatureSignerData <erbsland::conf::SignatureSignerData>` together with the source identifier and the
``signingPersonText`` supplied to ``Signer::sign()``.
The backend authenticates the exact digest text and returns the application-defined text that will be stored inside
``@signature``.

The compact backend below matches the validator from :doc:`validating-document-signatures`.
It authenticates the signer label and digest with HMAC-SHA-256 and encodes the result as Base64. The hard-coded demo key
only makes the documentation reproducible; a real shared-secret backend must load protected key material, while a
public-key workflow should keep the private key in protected storage or behind a signing service.

.. erbsland-demo::
    :source: conf/ConfigurationSignatures/HmacSignatureSigner.hpp
    :source-sha256: e0a3572b454fa7fecd579cbe38236bb40ea4e2792affec20456b49b00a502ecc

.. code-block:: cpp

    /// Create configuration signature text with a shared HMAC key.
    ///
    /// `SignatureSigner` receives the document digest in exactly the representation expected by the parser. This example
    /// authenticates the signer label and digest together, then stores the algorithm, label, and Base64 authenticator in
    /// the
    /// `@signature` text. A production backend can use the same interface with a private key, certificate, or signing
    /// service instead.
    class HmacSignatureSigner final : public el::conf::SignatureSigner {
    public:
        explicit HmacSignatureSigner(el::ByteBlock key) : _key{std::move(key)} { _key.markAsSensitive(); }

    public: // implement `SignatureSigner`
        [[nodiscard]] auto sign(const el::conf::SignatureSignerData &data) -> el::String override {
            if (data.signingPersonText.isEmpty() || data.signingPersonText.contains(";"_el)) {
                throw el::conf::ConfError{
                    el::conf::ConfErrorCategory::Signature,
                    "The signer label must be non-empty and must not contain a semicolon."_el};
            }

            auto authenticator = el::cryptology::Hmac{el::HashAlgorithm::Sha2_256, _key};
            authenticator.update(data.signingPersonText);
            authenticator.update("\n"_el);
            authenticator.update(data.documentDigest);
            const auto encoded =
                el::base_n::BaseNEncoder{authenticator.finalize(), el::base_n::BaseNFormat::base64()}.toString();
            return el::String::fromJoined({"hmac-sha256;"_el, data.signingPersonText, ";"_el, encoded});
        }

    private:
        el::ByteBlock _key;
    };

.. erbsland-demo-end::

``signingPersonText`` is input to your signature format, not proof of identity by itself.
In an interactive application, obtain it from the authenticated account or certificate rather than from an unrestricted
text field.
A backend that uses a certificate can place a certificate identifier in the signature text and let the validator derive
the human-readable identity from the verified certificate.

A Complete Parse, Sign, and Verify Workflow
===========================================

A small signing application needs only a source path, a destination path, an authenticated signer identity, and a
configured backend.
Its important ordering is easy to miss: first parse the unsigned source with the same syntax and validation policy used
by the application, then sign that unchanged file, and finally verify the produced file through the deployment parser.

The following demo separates the reusable ``signDocument()`` operation from its self-contained example setup.
It writes to another path so a failure cannot destroy the reviewed source, then verifies the result with the matching
strict validator.

.. erbsland-demo::
    :source: conf/ConfigurationSignatures/SignDocuments.cpp
    :exec: conf/configuration_signatures --demo SignDocuments
    :source-sha256: 21adf518216abec1a693e463c93556b891db97db8e7210f8576aae1d490760e5

.. code-block:: cpp

    /// Parse and sign the exact configuration file that will be deployed.
    ///
    /// `Signer` checks encoding and line limits but not ELCL syntax. Parsing first prevents a signing tool from approving a
    /// malformed document. Writing to a separate destination also leaves the reviewed source intact if signing fails.
    /// @return The parsed source document that was reviewed before signing.
    auto signDocument(const el::Path &sourcePath, const el::Path &destinationPath, const el::String &signingPerson)
        -> el::conf::DocumentPtr {
        const auto document = el::conf::Parser{}.parseFileOrThrow(sourcePath);
        auto backend = std::make_shared<HmacSignatureSigner>(signatureDemoKey());
        el::conf::Signer{backend}.sign(sourcePath, destinationPath, signingPerson);
        return document;
    }

    /// Run the complete signing and verification workflow for a configuration document.
    void signDocuments() {
        auto directoryOptions = el::PathTempDirectoryOptions{};
        directoryOptions.setPrefix("wadaiko-signing-"_el);
        const auto directory =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(directoryOptions);
        const auto unsignedPath = directory->path() / "ensemble.elcl"_el;
        const auto signedPath = directory->path() / "ensemble.signed.elcl"_el;
        unsignedPath.content().writeTextOrThrow(
            "[ensemble]\n"
            "Name: \"和太鼓 星\"\n"
            "Rehearsal Room: \"音楽室\"\n"_el);

        // Sign the validated source into a separate deployment file.
        const auto reviewedDocument = signDocument(unsignedPath, signedPath, "佐藤 美咲"_el);

        // Verify the output through the same parser path used by the application.
        auto parser = el::conf::Parser{};
        parser.setSignatureValidator(std::make_shared<HmacSignatureValidator>(signatureDemoKey(), "佐藤 美咲"_el));
        const auto document = parser.parseFileOrThrow(signedPath);
        el::io::printLine("Signed file: "_el, signedPath.name());
        el::io::printLine("Signer: 佐藤 美咲"_el);
        el::io::printLine("Reviewed ensemble: "_el, reviewedDocument->getTextOrThrow("ensemble.name"_el));
        el::io::printLine("Verified ensemble: "_el, document->getTextOrThrow("ensemble.name"_el));
    }

.. erbsland-ansi::
    :escape-char: ␛

    Signed file: ensemble.signed.elcl
    Signer: 佐藤 美咲
    Reviewed ensemble: 和太鼓 星
    Verified ensemble: 和太鼓 星

.. erbsland-demo-end::

In an application with validation rules, install those rules before the first parse.
If includes are allowed, remember that the parser validates each included file's signature independently under a strict
signature policy.
Sign every file that will be deployed, and perform the final root parse only after all signed files are in their
intended locations.

Keep the Reviewed Bytes Stable
==============================

Every byte below the signature line contributes to the digest.
Changing whitespace, comments, line endings, or the final newline invalidates the signature even when the resulting
value tree would be identical.
This byte-level rule is intentional: the signature describes a file that a person can inspect, archive, and compare, not
only the parser's interpretation of that file.

Generate configuration completely before approval, sign it as the final transformation, and copy the signed artifact
without text conversion.
Tools that automatically reformat files, normalize line endings, or append a newline must run before signing.
When an administrator edits a signed file, remove the old signature line while working and sign the finished document
again.

Choosing Where Signing Happens
==============================

The right integration depends less on the ``Signer`` API than on where people can safely reach the private key and the
configuration artifact.
Three patterns cover most deployments.

Signing in the Application
--------------------------

An application can expose a dedicated command such as ``--sign-configuration`` alongside its normal configuration
checks.
The validator, rules, source resolver, and signing command then evolve together, and the signing operation is always
available wherever the application is installed.
This is especially useful when the key resides on an attached token or is bound to the logged-in user.

The trade-off is operational reach.
Production hosts may be deliberately inaccessible, graphical applications may not work well over remote sessions, and
installing the complete application solely to sign a file can widen the trusted environment.
Keep signing as an explicit command that never starts the application's ordinary workload.

A Separate Local Tool
---------------------

A small signing utility can reuse the same parser, validation rules, and signer backend without carrying the rest of the
application.
The private key stays on an administrator's workstation or hardware device, and the tool can be updated independently of
the deployed service.

This model requires the configuration to travel to the workstation and the signed result to travel back.
Use a transfer path that preserves bytes, give the operator a clear preview of the file being approved, and verify the
returned artifact before deployment.
Version the validation policy with the application so an old tool cannot approve a configuration that the current
application interprets differently.

A Web Signing Service
---------------------

A web service can bind signing authority to an enterprise login and keep private keys in centrally managed hardware.
It is often reachable from environments where local tools or device access are restricted.
The service can display the submitted configuration, record the approval, and return the signed document as a download.

The configuration content must then be sent to the service, which may be unacceptable for sensitive deployments.
The service also becomes an availability and security dependency, so it needs access controls, audit records, key
rotation, request-size limits, and a way for the user to confirm the exact bytes being signed.
Return the complete signed file rather than reconstructing it in the browser.

Whichever model you choose, the application remains the final authority.
It should validate the signature every time it loads the configuration and reject a signed artifact whose identity,
algorithm, key status, or document digest no longer satisfies the current trust policy.
