// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/CompressionLevel.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringEncoder.hpp>
#include <erbsland/StringEncoding.hpp>
#include <erbsland/TempDirectory.hpp>
#include <erbsland/zip/ArchiveWriter.hpp>
#include <erbsland/zip/CompressionMethod.hpp>

namespace demo {

/// Create and finalize a ZIP archive from bytes using persistent compression defaults.
void writeArchive() {
    const auto workspace = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    const auto archivePath = workspace->path() / "field-notes.zip"_el;
    const auto notesText = el::String{"alpine observation"_el};
    const auto notes = el::StringEncoder{notesText}.encode(el::StringEncoding::Utf8);

    auto writer = el::zip::ArchiveWriter::create(archivePath);
    writer->setCompressionMethod(el::zip::CompressionMethod::Zstandard);
    writer->setCompressionLevel(el::CompressionLevel::High);
    writer->setComment("Expedition field notes"_el);
    writer->addData(notes, el::Path{"observations/day-01.txt"_el});
    writer->finalize();

    el::io::printLine("Archive : "_el, archivePath.toString());
    el::io::printLine("Bytes   : "_el, archivePath.info().fileSize().toSizeT());
}

}
