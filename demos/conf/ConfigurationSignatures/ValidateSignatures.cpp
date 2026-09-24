// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationSignaturesDemos.hpp"
#include "HmacSignatureSigner.hpp"
#include "HmacSignatureValidator.hpp"
#include "SignatureDemoSupport.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Signer.hpp>

namespace demo {

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

}
