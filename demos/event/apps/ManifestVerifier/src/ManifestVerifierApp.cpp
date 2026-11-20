// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ManifestVerifierApp.hpp"

#include "VerificationSession.hpp"

#include <erbsland/Options.hpp>
#include <erbsland/OptionValues.hpp>
#include <erbsland/StandardStreams.hpp>

namespace demo {

void ManifestVerifierApp::initialize() {
    info().setApplicationName("Manifest Verifier"_el);
}

void ManifestVerifierApp::registerCommandLineOptions(const el::OptionsPtr &options) {
    options->setHelpDescription("Verify the SHA-256 digests in a manifest."_el);
    options->addOption(cManifest)
        .setRequired()
        .setHelpTitle("Checksum manifest"_el)
        .setHelpDescription("SHA-256 digest, two spaces and a relative file name on each line."_el);
    options->addOption({"--fail-fast"_el, cFailFast})
        .setType(el::OptionType::Flag)
        .setHelpTitle("Stop on mismatch"_el)
        .setHelpDescription("Report the first mismatched file and how many entries remain unchecked."_el);
}

void ManifestVerifierApp::handleCommandLineOptions(const el::OptionValuesPtr &values) {
    _manifest = el::Path::fromNativeOrThrow(values->getText(cManifest)).toAbsoluteOrThrow();
    _failFast = values->getFlag(cFailFast);
}

auto ManifestVerifierApp::main() -> el::ExitCode {
    auto session = VerificationSession{_manifest, _failFast};
    if (session.run()) {
        return el::ExitCode::success();
    }
    return el::ExitCode::failure();
}

}
