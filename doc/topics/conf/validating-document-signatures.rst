..
    Copyright (c) 2026 Tobias Erbsland - Erbsland DEV. https://erbsland.dev
    SPDX-License-Identifier: Apache-2.0

.. index::
    single: Configuration; Signature Validation
    single: Document Signatures; Validation
    single: SignatureValidator
    single: Security; Signed Configuration

***********************************
Validating Configuration Signatures
***********************************

A signed configuration is useful only when the application can connect its signature to an independent source of trust.
This page turns that trust decision into a
:cpp:class:`SignatureValidator <erbsland::conf::SignatureValidator>`, installs it in the parser, and shows how one policy
can either require signatures or deliberately admit unsigned development files.
You will also see why the validator receives the document digest as text and how modified, malformed, and unsigned
documents reach the same decision point.

The Validator Is Your Trust Policy
==================================

The parser understands where an ELCL signature belongs and which document bytes it covers, but it cannot know whom your
application trusts.
That decision belongs in ``SignatureValidator::validate()``.
The parser calls this method once for every source that it finishes reading, including the root document, every included
document, and documents without an ``@signature`` line.

Each call receives :cpp:struct:`SignatureValidatorData <erbsland::conf::SignatureValidatorData>` with three values:

``sourceIdentifier``
    Identifies the source being checked.
    A policy can use it when different sources have different trust requirements.

``signatureText``
    Contains the decoded text from ``@signature``, without the surrounding ELCL quotes.
    It is empty when the document has no signature line.

``documentDigest``
    Contains the parser's digest in its complete textual form, including the algorithm name.
    Authenticate this exact string; do not extract and normalize its hexadecimal part.

The implementation interprets the application-owned signature format, establishes the signer identity, verifies the
cryptographic evidence, and returns
:cpp:enumerator:`SignatureValidatorResult::Accept <erbsland::conf::SignatureValidatorResult::Accept>` only when every
part of its policy succeeds.
Returning
:cpp:enumerator:`SignatureValidatorResult::Reject <erbsland::conf::SignatureValidatorResult::Reject>` asks the parser to
report the standard signature error.
When an operator needs a more precise explanation, the validator may instead throw a
:cpp:class:`ConfError <erbsland::conf::ConfError>` with the ``Signature`` category.

A Compact Shared-Secret Validator
=================================

The next implementation uses :cpp:class:`Hmac <erbsland::cryptology::Hmac>` so the complete example stays small and can
be compiled without an external certificate store.
Its signature text has three fields: the algorithm name, a signer label, and a Base64-encoded authenticator.
Both the label and the document digest are authenticated, and ``Hmac::verify()`` performs the final comparison without
content-dependent short-circuiting.

HMAC uses one shared secret for signing and verification.
That makes it suitable for applications in which both operations belong to the same protected system, but it does not
provide the separation of authority offered by a private signing key and a public verification key.
For production workflows with individual signers, implement the same ``SignatureValidator`` interface using your
certificate, public-key, hardware-token, or signing-service policy.

.. erbsland-demo::
    :source: conf/ConfigurationSignatures/HmacSignatureValidator.hpp
    :source-sha256: 15c09bf77a38b331c7b0af4335628b4401e45e4f6273506622effb44b2903cc3

.. code-block:: cpp

    /// Validate configuration signatures with a trusted signer label and shared HMAC key.
    ///
    /// The parser calls `validate()` for every source, even when its signature text is empty. The policy can therefore
    /// require signatures for deployed files or admit unsigned files in a controlled development mode. Signed input is
    /// accepted only when its format, signer label, and constant-time HMAC verification all succeed.
    class HmacSignatureValidator final : public el::conf::SignatureValidator {
    public:
        enum class UnsignedDocuments : uint8_t {
            Reject,
            Accept,
        };

    public:
        HmacSignatureValidator(
            el::ByteBlock key,
            el::String trustedSigner,
            const UnsignedDocuments unsignedDocuments = UnsignedDocuments::Reject) :
            _key{std::move(key)}, _trustedSigner{std::move(trustedSigner)}, _unsignedDocuments{unsignedDocuments} {
            _key.markAsSensitive();
        }

    public: // implement `SignatureValidator`
        [[nodiscard]] auto validate(const el::conf::SignatureValidatorData &data)
            -> el::conf::SignatureValidatorResult override {
            using Result = el::conf::SignatureValidatorResult;
            if (data.signatureText.isEmpty()) {
                return _unsignedDocuments == UnsignedDocuments::Accept ? Result::Accept : Result::Reject;
            }

            const auto algorithmSeparator = data.signatureText.find(";"_el);
            if (algorithmSeparator.isNoIndex()) {
                return Result::Reject;
            }
            auto [algorithm, payload] = data.signatureText.splitAt(algorithmSeparator);
            payload = std::get<1>(payload.slice(el::StringSide::Front));

            const auto signerSeparator = payload.find(";"_el);
            if (signerSeparator.isNoIndex()) {
                return Result::Reject;
            }
            auto [signer, authenticatorText] = payload.splitAt(signerSeparator);
            authenticatorText = std::get<1>(authenticatorText.slice(el::StringSide::Front));
            if (algorithm != "hmac-sha256"_el || signer != _trustedSigner) {
                return Result::Reject;
            }

            const auto expected =
                el::text::base_n::BaseNDecoder{authenticatorText, el::text::base_n::BaseNFormat::base64()}.toData(
                    el::ByteLength{32U});
            if (!expected.has_value()) {
                return Result::Reject;
            }
            auto authenticator = el::cryptology::Hmac{el::HashAlgorithm::Sha2_256, _key};
            authenticator.update(signer);
            authenticator.update("\n"_el);
            authenticator.update(data.documentDigest);
            return authenticator.verify(expected.value()) ? Result::Accept : Result::Reject;
        }

    private:
        el::ByteBlock _key;
        el::String _trustedSigner;
        UnsignedDocuments _unsignedDocuments;
    };

.. erbsland-demo-end::

The signer label is not trusted merely because it appears in the signature text.
It becomes meaningful only because it is included in the authenticated message and compared with a label from the
validator's independent policy.
A certificate-based implementation would derive the identity from the verified certificate instead, then apply its
authorization rules to that identity.

Enforcing Validation During Parsing
===================================

Create the validator before reading configuration and install it with
:cpp:func:`Parser::setSignatureValidator() <erbsland::conf::Parser::setSignatureValidator>`.
A parser without a validator accepts ordinary unsigned input, but rejects signed input because it has no way to
establish its authenticity.
Once a validator is installed, its result decides whether each source is accepted.

This demo signs a small percussion-ensemble configuration, parses it successfully, changes one value after signing, and
then shows the parser rejecting the changed copy.
The final parse uses an explicitly relaxed development policy for an unsigned file.

.. erbsland-demo::
    :source: conf/ConfigurationSignatures/ValidateSignatures.cpp
    :exec: conf/configuration_signatures --demo ValidateSignatures
    :source-sha256: 7eff652f3c4999d8e59542e81484379426dbebf7e3d77a0afbf50cc0c7f41960

.. code-block:: cpp

    /// Enforce signatures while parsing and reject a document that changed after signing.
    ///
    /// Install a `SignatureValidator` before parsing any protected source. A strict validator rejects unsigned input as
    /// well as malformed or incorrect signatures. A separately configured development parser can deliberately accept
    /// unsigned input while continuing to verify every signature it encounters.
    void validateSignatures() {
        auto directoryOptions = el::PathTempDirectoryOptions{};
        directoryOptions.setPrefix("wadaiko-signature-"_el);
        const auto directory =
            el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow(directoryOptions);
        const auto unsignedPath = directory->path() / "ensemble.elcl"_el;
        const auto signedPath = directory->path() / "ensemble.signed.elcl"_el;
        const auto changedPath = directory->path() / "ensemble.changed.elcl"_el;
        unsignedPath.content().writeTextOrThrow(
            "[ensemble]\n"
            "Name: \"和太鼓 星\"\n"
            "Performers: 12\n"_el);

        // Prepare a signed file so the example can exercise both successful and rejected validation.
        el::conf::Signer{std::make_shared<HmacSignatureSigner>(signatureDemoKey())}.sign(
            unsignedPath, signedPath, "佐藤 美咲"_el);

        // A strict parser accepts the authentic document.
        auto parser = el::conf::Parser{};
        parser.setSignatureValidator(std::make_shared<HmacSignatureValidator>(signatureDemoKey(), "佐藤 美咲"_el));
        const auto document = parser.parseFileOrThrow(signedPath);
        el::io::printLine("Signed document: accepted ("_el, document->getTextOrThrow("ensemble.name"_el), ")"_el);

        // Any change after signing produces another digest and is rejected.
        const auto changedText =
            signedPath.content().readTextOrThrow().replacedFirst("Performers: 12"_el, "Performers: 16"_el);
        changedPath.content().writeTextOrThrow(changedText);
        const auto changedWasRejected = !parser.parseFile(changedPath);
        el::io::printLine("Changed document: "_el, changedWasRejected ? "rejected"_el : "accepted"_el);

        // Development mode can admit unsigned documents through an explicit validator policy.
        auto developmentParser = el::conf::Parser{};
        developmentParser.setSignatureValidator(
            std::make_shared<HmacSignatureValidator>(
                signatureDemoKey(), "佐藤 美咲"_el, HmacSignatureValidator::UnsignedDocuments::Accept));
        const auto unsignedWasAccepted = developmentParser.parseFile(unsignedPath) != nullptr;
        el::io::printLine("Unsigned development document: "_el, unsignedWasAccepted ? "accepted"_el : "rejected"_el);
    }

.. erbsland-ansi::
    :escape-char: ␛

    Signed document: accepted (和太鼓 星)
    Changed document: rejected
    Unsigned development document: accepted

.. erbsland-demo-end::

The non-throwing ``parseFile()`` call returns a null document for the changed file and leaves the complete diagnostic in
:cpp:func:`Parser::lastError() <erbsland::conf::Parser::lastError>`.
Use ``parseFileOrThrow()`` when signature failure should follow the same exception path as syntax, access, or validation
errors.

Strict and Development Policies
===============================

A strict deployment validator rejects an empty ``signatureText``.
Because the callback runs for every included document, this requires the root and all includes to be signed separately.
That is usually the clearest model: every file retains its own approval evidence, and replacing an include cannot hide
behind an authentic root document.

Development often needs a different balance.
The example's ``UnsignedDocuments::Accept`` mode accepts an empty signature while still verifying every signature that
is present.
This is safer than omitting the validator when developers regularly test signed files, because a damaged signed file
does not silently become ordinary input.

Keep the choice between strict and development policy outside the configuration being checked.
A protected application setting, deployment entitlement, or deliberate build mode can make that choice.
An option inside the same document would allow an attacker to disable the condition that is meant to detect the edit.

Failure Should Be Uneventful
============================

Signature text is untrusted input.
Treat unknown algorithms, missing fields, invalid Base64, unexpected signer identities, wrong signature lengths, and
cryptographic mismatches as ordinary rejection paths.
Place limits on decoded data before allocating it, as the demo does with the expected 32-byte HMAC size, and avoid
including secrets or raw key material in diagnostics.

Key rotation deserves an explicit policy rather than a special parsing rule.
A signature format can carry a small key or certificate identifier, and the validator can use that identifier to select
one of several already trusted public keys.
Remove an old key from this trusted set only after all documents signed with it have been replaced.

The next topic, :doc:`signing-configuration-documents`, implements the other side of this contract and builds a complete
parse, sign, and verify workflow.
