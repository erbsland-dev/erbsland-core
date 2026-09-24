// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ConfigurationSignaturesDemos.hpp"
#include "HmacSignatureSigner.hpp"
#include "HmacSignatureValidator.hpp"
#include "SignatureDemoSupport.hpp"

#include <erbsland/conf/Parser.hpp>
#include <erbsland/conf/Signer.hpp>

namespace demo {

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

}
