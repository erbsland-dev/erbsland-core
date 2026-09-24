// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveWriter.hpp"

#include "ZipError.hpp"

#include "impl/ArchiveWriter.hpp"

#include "../../err/ParameterError.hpp"
#include "../../path/PathError.hpp"
#include "../../path/PathOperations.hpp"
#include "../../path/PathTempFileOptions.hpp"
#include "../../stream/TempByteOutputStream.hpp"
#include "../../text/Literals.hpp"

#include <memory>
#include <utility>

namespace erbsland::compression::zip {

using namespace text::literals;

auto ArchiveWriter::create(const path::Path &destination, ArchiveWriterOptions options) -> ArchiveWriterPtr {
    if (destination.isEmpty()) {
        throw err::ParameterError{"The ZIP archive destination path must not be empty."_el, "destination"_el};
    }
    try {
        auto parent = destination.parent();
        if (parent.isEmpty()) {
            parent = path::Path::fromPosix("."_el);
        }
        auto temporary = parent.operations().openTempByteOutputStreamOrThrow(
            path::PathTempFileOptions{}
                .setStreamSettings(options.outputStreamSettings())
                .setPrefix(".zip-archive-"_el)
                .setSuffix(".tmp"_el));
        return std::make_shared<impl::ArchiveWriter>(temporary, temporary, destination, std::move(options));
    } catch (const ZipError &) {
        throw;
    } catch (const path::PathError &) {
        throw ZipError{
            ZipErrorContext{
                ZipErrorReason::PathFailure,
                ZipOperationPhase::Open,
                "ZIP archive destination could not be opened"_el,
                "A sibling temporary archive could not be created for the destination path."_el}
                .setDestinationPath(destination),
            std::current_exception()};
    }
}

auto ArchiveWriter::create(stream::ByteOutputStreamPtr destination, ArchiveWriterOptions options) -> ArchiveWriterPtr {
    return std::make_shared<impl::ArchiveWriter>(std::move(destination), nullptr, path::Path{}, std::move(options));
}

}
