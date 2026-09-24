// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/compression/zip/ArchiveItem.hpp>
#include <erbsland/compression/zip/ArchiveReader.hpp>
#include <erbsland/compression/zip/ArchiveWriter.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/TempDirectory.hpp>
#include <erbsland/text/StringEncoder.hpp>

namespace demo {

/// Inspect validated ZIP metadata and extract selected entries under a safe root.
void readArchive() {
    const auto workspace = el::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
    const auto archivePath = workspace->path() / "survey.zip"_el;
    const auto northText = el::String{"north"_el};
    const auto southText = el::String{"south"_el};
    auto writer = el::zip::ArchiveWriter::create(archivePath);
    writer->addData(
        el::StringEncoder{northText}.encode(el::StringEncoding::Utf8), el::Path{"measurements/north.txt"_el});
    writer->addData(
        el::StringEncoder{southText}.encode(el::StringEncoding::Utf8), el::Path{"measurements/south.txt"_el});
    writer->finalize();

    const auto outputRoot = workspace->path() / "selected"_el;
    auto reader = el::zip::ArchiveReader::create(archivePath);
    for (const auto &item : reader->items()) {
        el::io::printLine(item->path().toString(), " : "_el, item->uncompressedLength().toSizeT(), " bytes"_el);
    }
    reader->extractToDirectory(
        outputRoot, {}, [](const el::zip::ArchiveItem &item) { return item.path().name() == "north.txt"_el; });
    reader->close();

    el::io::printLine("Extracted under: "_el, outputRoot.toString());
}

}
