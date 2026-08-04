// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Signer.hpp"

#include "../char/CharStream.hpp"
#include "../char/NamedChars.hpp"
#include "../constants/Defaults.hpp"
#include "../constants/Limits.hpp"

#include "../../../path/PathContent.hpp"
#include "../../../path/PathError.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../path/PathWriteTextOptions.hpp"
#include "../../../text/EscapeFormat.hpp"
#include "../../../text/Literals.hpp"
#include "../../../text/StringEditor.hpp"
#include "../../ConfError.hpp"
#include "../../SignatureSigner.hpp"
#include "../../SignatureSignerData.hpp"
#include "../../Source.hpp"

namespace erbsland::conf::impl {

using namespace text::literals;

Signer::Signer(SignatureSignerPtr signatureSigner) : _signatureSigner{std::move(signatureSigner)} {
    if (_signatureSigner == nullptr) {
        throw err::ParameterError("Signature signer must not be null"_el, "signatureSigner"_el);
    }
}

void Signer::sign(path::Path sourcePath, path::Path destinationPath, text::String signingPersonText) {
    auto source = validateAndCreateSource(sourcePath);
    auto [digestText, digest, hasWindowsLineBreaks] = buildDigest(source);
    const SignatureSignerData data{
        .sourceIdentifier = source->identifier(),
        .signingPersonText = std::move(signingPersonText),
        .documentDigest = std::move(digestText)};
    source = {};
    auto signatureText = _signatureSigner->sign(data);
    validateAndEscapeSignatureText(signatureText);
    writeSignedFile(sourcePath, destinationPath, signatureText, digest, hasWindowsLineBreaks);
}

auto Signer::validateAndCreateSource(path::Path sourcePath) -> SourcePtr {
    try {
        sourcePath = sourcePath.resolveOrThrow(path::PathResolveMode::Physical);
        auto sourceInfo = sourcePath.info();
        sourceInfo.reload(path::PathInfoParts{path::PathInfoPart::Type, path::PathInfoPart::Size});
        if (!sourceInfo.isRegularFile()) {
            throw ConfError{ConfErrorCategory::IO, "The source path is no existing regular file."_el, sourcePath};
        }
        if (sourceInfo.fileSize().toSizeT() > limits::maxDocumentSize) {
            throw ConfError{ConfErrorCategory::LimitExceeded, "The source file is too large."_el, sourcePath};
        }
    } catch (const path::PathError &) {
        throw ConfError{
            ConfErrorCategory::IO,
            "Validating the Configuration Source Failed"_el,
            "The source file location or size could not be validated."_el,
            sourcePath,
            std::current_exception()};
    }
    return Source::fromFile(sourcePath);
}

auto Signer::buildDigest(const SourcePtr &source) -> DigestResult {
    source->open();
    CharStream charStream{source};
    charStream.enableHash();
    // read the whole file to verify it's encoding, line lengths and to calculate its hash value.
    auto c = charStream.next();
    bool hasWindowsLineEndings = false;
    while (!c.character().isEndOfData()) {
        c = charStream.next();
        if (!hasWindowsLineEndings && c.character() == nc::carriageReturn) {
            hasWindowsLineEndings = true;
        }
    }
    auto digest = charStream.digest();
    auto digestText = text::String::fromJoined({
        defaults::documentHashAlgorithm.toString(),
        " "_el,
        text::String::fromByteBlock(digest, text::ByteFormat::compact()),
    });
    source->close();
    return {
        .digestText = std::move(digestText),
        .digest = std::move(digest),
        .hasWindowsLineEndings = hasWindowsLineEndings};
}

void Signer::validateAndEscapeSignatureText(text::String &signatureText) {
    if (signatureText.isEmpty()) {
        throw ConfError{ConfErrorCategory::Signature, "The signature text is empty."_el};
    }
    signatureText = signatureText.toEscaped(text::EscapeFormat::Config, text::EscapeAmount::Required);
    if (signatureText.length().toSizeT() > limits::maxLineLength - 20) {
        throw ConfError{ConfErrorCategory::LimitExceeded, "The signature text is too long."_el};
    }
}

void Signer::writeSignedFile(
    const path::Path &sourcePath,
    const path::Path &destinationPath,
    const text::String &digestText,
    const mem::ByteBlock &digest,
    bool hasWindowsLineBreaks) {
    try {
        auto output = text::StringEditor{};
        output.reserve(unit::ByteLength{limits::maxLineLength + 50U});
        output.append("@signature: \""_el).append(digestText).append(U'"');
        output.append(hasWindowsLineBreaks ? "\r\n"_el : "\n"_el);
        const auto digestAfterWrite = writeConfiguration(output, sourcePath);
        if (digest != digestAfterWrite) {
            throw ConfError{
                ConfErrorCategory::Signature, "The source file has been modified while writing the signed version."_el};
        }
        destinationPath.content().writeTextOrThrow(
            text::String{output}, path::PathWriteTextOptions{text::StringEncoding::Utf8});
    } catch (const ConfError &) {
        throw;
    } catch (const path::PathError &) {
        throw ConfError{
            ConfErrorCategory::IO,
            "Writing the Signed Configuration Failed"_el,
            "The destination file could not be written."_el,
            destinationPath,
            std::current_exception()};
    }
}

auto Signer::writeConfiguration(text::StringEditor &output, const path::Path &sourcePath) -> mem::ByteBlock {
    // Reopen the source.
    auto source = Source::fromFile(sourcePath);
    source->open();
    CharStream charStream{source};
    charStream.enableHash();
    auto character = charStream.next();
    // Check if the first line is a signature line and skip it.
    if (charStream.isSignatureLine()) {
        while (character.character() != nc::newLine && !character.character().isEndOfData()) {
            character = charStream.next();
        }
        if (!character.character().isEndOfData()) {
            character = charStream.next();
        }
    }
    while (!character.character().isEndOfData()) {
        output.append(character.character());
        character = charStream.next();
    }
    return charStream.digest();
}

}
